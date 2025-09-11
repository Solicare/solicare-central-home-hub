#include "server/async_websocket_server.hpp"
#include "utils/logging_utils.hpp"

using namespace std;
using namespace std::chrono;
using namespace boost::asio;

using namespace WebSocketServerContext;

namespace
{
constexpr std::string_view TAG = "AsyncWebSocketServer";
constexpr auto CONSOLE_COLOR   = Logger::ConsoleColor::BLUE;
} // namespace

// Initialize static members
bool WebSocketServerContext::flag_stop_server = false;
SessionMap WebSocketServerContext::ws_session_map;
ServerConfig WebSocketServerContext::ws_server_config;
TimePoint WebSocketServerContext::ws_last_session_logged;

AsyncWebSocketServer::AsyncWebSocketServer(io_context& io_context)
    : socket_acceptor_(make_shared<AsyncSocketAcceptor>(io_context))
{
}

void AsyncWebSocketServer::start()
{
	socket_acceptor_->do_accept();
	periodic_session_manage_thread_ = make_shared<jthread>(
	    [](const stop_token& st)
	    {
		    ws_last_session_logged = steady_clock::now();
		    while (!st.stop_requested())
		    {
			    on_session_manage();
			    this_thread::sleep_for(milliseconds(ws_server_config.session_manage_period_ms));
		    }
	    });
	Logger::log_info(
	    TAG,
	    fmt::format("Server started successfully on port {}", socket_acceptor_->acceptor_->local_endpoint().port()),
	    CONSOLE_COLOR);
}

void AsyncWebSocketServer::stop()
{
	flag_stop_server = true;
	if (periodic_session_manage_thread_)
	{
		periodic_session_manage_thread_->request_stop();
	}
	if (socket_acceptor_ && socket_acceptor_->acceptor_ && socket_acceptor_->acceptor_->is_open())
	{
		socket_acceptor_->acceptor_->close();
	}
	ws_session_map.cvisit_all(
	    [&](const auto& pair)
	    {
		    const auto& session = pair.second;
		    if (session && session->ws && session->ws->is_open() && session->ws->next_layer().is_open())
		    {
			    const auto device_ip = session->ws->next_layer().remote_endpoint().address().to_string();
			    session->ws->async_close(
			        boost::beast::websocket::close_code::normal,
			        [device_ip](const boost::system::error_code& ec)
			        {
				        if (!ec)
				        {
					        Logger::log_info(TAG, fmt::format("[Close] WebSocket closed for device_ip={}", device_ip),
					                         CONSOLE_COLOR);
				        }
				        else
				        {
					        Logger::log_error(TAG, fmt::format("[Close] Error closing WebSocket for device_ip={}: {}",
					                                           device_ip, ec.message()));
				        }
			        });
		    }
	    });
	ws_session_map.cvisit_all(
	    [&](const auto& pair)
	    {
		    const auto& session = pair.second;
		    while (session && session->ws && session->ws->is_open() && session->ws->next_layer().is_open())
		    {
			    std::this_thread::sleep_for(milliseconds(10));
		    }
	    });
	std::this_thread::sleep_for(milliseconds(1000));
	Logger::log_info(
	    TAG, fmt::format("WebSocket server on port {} has been stopped successfully.", ws_server_config.server_port),
	    CONSOLE_COLOR);
}

AsyncSocketAcceptor::AsyncSocketAcceptor(io_context& io_context)
    : io_context_(io_context), acceptor_(make_shared<ip::tcp::acceptor>(
                                   io_context, ip::tcp::endpoint(ip::tcp::v4(), ws_server_config.server_port)))
{
}

void AsyncSocketAcceptor::do_accept()
{
	auto socket = make_shared<Socket>(io_context_);
	if (acceptor_ && acceptor_->is_open())
	{
		acceptor_->async_accept(
		    *socket,
		    [this, socket](const boost::system::error_code& ec)
		    {
			    if (socket && socket->is_open())
			    {
				    if (!ec)
				    {
					    const auto connection = make_shared<AsyncWebSocketConnection>(socket);
					    connection->upgrade();
				    }
				    else if (!flag_stop_server)
				    {
					    Logger::log_error(TAG, fmt::format("[Accept] Error: remote <{}:{}>, error: {} ({})",
					                                       socket->remote_endpoint().address().to_string(),
					                                       socket->remote_endpoint().port(), ec.message(), ec.value()));
				    }
				    do_accept();
			    }
			    else if (!flag_stop_server)
			    {
				    Logger::log_error(TAG, "[Accept] Error: Socket connection is already closed.");
			    }
		    });
	}
	else if (!flag_stop_server)
	{
		Logger::log_error(TAG, "[Accept] Error: Cannot attempt to accept connection because the acceptor is closed.");
	}
}

AsyncWebSocketConnection::AsyncWebSocketConnection(const shared_ptr<Socket>& socket)
    : WebSocketConnection(make_shared<WebSocket>(std::move(*socket)))
{
}

void AsyncWebSocketConnection::upgrade()
{
	const auto self = shared_from_this();
	if (ws_ && ws_->next_layer().is_open())
	{
		ws_->async_accept(
		    [self](const boost::system::error_code& ec)
		    {
			    if (self->ws_ && self->ws_->is_open() && self->ws_->next_layer().is_open())
			    {
				    if (!ec)
				    {
					    self->do_read();
				    }
				    else if (!flag_stop_server)
				    {
					    const auto remote_endpoint = self->ws_->next_layer().remote_endpoint();
					    Logger::log_error(TAG, fmt::format("[Upgrade] Error: remote <{}:{}>, error: {} ({})",
					                                       remote_endpoint.address().to_string(),
					                                       remote_endpoint.port(), ec.message(), ec.value()));
				    }
			    }
			    else if (!flag_stop_server)
			    {
				    Logger::log_error(TAG, "[Upgrade] Error: Socket connection is already closed.");
			    }
		    });
	}
	else if (!flag_stop_server)
	{
		Logger::log_error(
		    TAG, "[Upgrade] Error: Cannot attempt to upgrade because the socket connection is already closed.");
	}
}

void AsyncWebSocketConnection::do_read()
{
	auto buffer     = make_shared<Buffer>();
	const auto self = shared_from_this();
	if (ws_ && ws_->is_open() && ws_->next_layer().is_open())
	{
		ws_->async_read(
		    *buffer,
		    [self, buffer](const boost::system::error_code& ec, const size_t)
		    {
			    if (self->ws_ && self->ws_->is_open() && self->ws_->next_layer().is_open())
			    {
				    const std::string device_ip = self->ws_->next_layer().remote_endpoint().address().to_string();
				    if (!ec)
				    {
					    if (ws_session_map.contains(device_ip))
					    {
						    ws_session_map.visit(device_ip,
						                         [&](const auto& pair)
						                         {
							                         const auto& session = pair.second;
							                         on_session_read(session, buffer);
						                         });
					    }
					    else
					    {
						    auto session = std::make_shared<Session>(self->ws_);
						    ws_session_map.insert_or_assign(device_ip, session);
						    on_session_created(session, buffer);
					    }
				    }
				    else
				    {
					    Logger::log_error(TAG,
					                      fmt::format("[Read] Error: remote: <{}:{}>, error: {} ({})",
					                                  self->ws_->next_layer().remote_endpoint().address().to_string(),
					                                  self->ws_->next_layer().remote_endpoint().port(), ec.message(),
					                                  ec.value()));
				    }
				    self->do_read();
			    }
			    else if (!flag_stop_server)
				    Logger::log_error(TAG, "[Read] Error: WebSocket connection is already closed. The session will be "
				                           "removed by session management.");
		    });
	}
	else if (!flag_stop_server)
	{
		Logger::log_error(TAG,
		                  "[Read] Error: Cannot attempt to read because the WebSocket connection is already closed.");
	}
}
