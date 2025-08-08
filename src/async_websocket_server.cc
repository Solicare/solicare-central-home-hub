#include <cstring>
#include <format>
#include <iostream>

#include "server/async_websocket_server.h"

using namespace std;
using namespace std::chrono;
using namespace boost::asio;

AsyncWebSocketServer::AsyncWebSocketServer(io_context& io_context, unsigned short port)
	: socket_acceptor_(make_shared<AsyncSocketAcceptor>(io_context, port))
{
}

void AsyncWebSocketServer::start()
{
	socket_acceptor_->do_accept();
	periodic_session_manage_thread_ = make_shared<jthread>(
		[](const stop_token& st)
		{
			while (!st.stop_requested())
			{
				on_session_manage();
				this_thread::sleep_for(milliseconds(SESSION_MANAGE_PERIOD_MS));
			}
		});
	cout << "[AsyncWebSocketServer] Server started successfully on port "
		<< socket_acceptor_->acceptor_->local_endpoint().port() << endl;
}

AsyncWebSocketServer::AsyncSocketAcceptor::AsyncSocketAcceptor(io_context& io_context, const unsigned short port)
	: io_context_(io_context),
	  acceptor_(make_shared<ip::tcp::acceptor>(io_context, ip::tcp::endpoint(ip::tcp::v4(), port)))
{
}

void AsyncWebSocketServer::AsyncSocketAcceptor::do_accept()
{
	auto socket = make_shared<Socket>(io_context_);
	acceptor_->async_accept(
		*socket,
		[this, socket](const boost::system::error_code& ec)
		{
			if (!ec)
			{
				const auto connection = make_shared<AsyncWebSocketConnection>(socket);
				connection->upgrade();
			}
			else
			{
				cerr << format("[SocketAcceptor][AcceptError] error: {} ({})", ec.message(), ec.value()) << endl;
			}
			do_accept();
		});
}

AsyncWebSocketConnection::AsyncWebSocketConnection(const shared_ptr<Socket>& socket)
	: WebSocketConnection(make_shared<WebSocket>(move(*socket)))
{
}

void AsyncWebSocketConnection::upgrade()
{
	const auto self = shared_from_this();
	ws_->async_accept(
		[self](const boost::system::error_code& ec)
		{
			if (!ec)
			{
				self->do_read();
			}
			else
			{
				const auto remote_endpoint = self->ws_->next_layer().remote_endpoint();
				cerr << format("[WebSocket][UpgradeError] remote: <{}:{}>, error: {} ({})",
				               remote_endpoint.address().to_string(), remote_endpoint.port(), ec.message(), ec.value())
					<< endl;
			}
		});
}

void AsyncWebSocketConnection::do_read()
{
	auto buffer     = make_shared<Buffer>();
	const auto self = shared_from_this();
	ws_->async_read(*buffer,
	                [self, buffer](const boost::system::error_code& ec, const size_t)
	                {
		                if (!ec)
		                {
			                on_read(self->ws_, buffer);
			                self->do_read();
		                }
		                else
		                {
			                const auto remote_endpoint = self->ws_->next_layer().remote_endpoint();
			                cerr << format("[WebSocket][ReadError] remote: <{}:{}>, error: {} ({})",
			                               remote_endpoint.address().to_string(), remote_endpoint.port(), ec.message(),
			                               ec.value())
				                << endl;
		                }
	                });
}
