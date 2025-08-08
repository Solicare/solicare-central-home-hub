#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <memory>

#define MAX_SESSION 100
#define SESSION_TIMEOUT 5            // seconds
#define SESSION_MANAGE_PERIOD_MS 500 // milliseconds

struct Session;
struct SessionInfo;

typedef std::chrono::steady_clock::duration Duration;
typedef std::chrono::steady_clock::time_point TimePoint;

typedef boost::asio::ip::tcp::socket Socket;
typedef boost::beast::flat_buffer Buffer;
typedef boost::beast::websocket::stream<Socket> WebSocket;
typedef boost::concurrent_flat_map<std::string, std::shared_ptr<Session>> SessionMap;

struct Session
{
	Session(const std::shared_ptr<WebSocket>& websocket, const std::shared_ptr<Buffer>& buffer);

	const std::shared_ptr<WebSocket> ws;
	std::shared_ptr<SessionInfo> info;
};

inline SessionMap sessions(MAX_SESSION);
inline void on_read(const std::shared_ptr<WebSocket>& websocket, const std::shared_ptr<Buffer>& buffer)
{
	const std::string device_ip = websocket->next_layer().remote_endpoint().address().to_string();
	if (sessions.contains(device_ip))
	{
		sessions.visit(device_ip,
		               [&](const auto& pair)
		               {
			               const auto& session = pair.second;
			               process_received_data(session, buffer);
		               });
		return;
	}

	auto session       = std::make_shared<Session>(websocket, buffer);
	const auto message = boost::beast::buffers_to_string(buffer->data());
	sessions.insert_or_assign(device_ip, session);
	std::cout << std::format("[Session][Created] new session for device: {} ({})", message, device_ip) << std::endl;
}

class WebSocketServer
{
  public:
	virtual ~WebSocketServer() = default;
	virtual void start()       = 0;

  protected:
	WebSocketServer() = default;
	std::shared_ptr<std::jthread> periodic_session_manage_thread_;
};

class WebSocketConnection
{
  public:
	virtual ~WebSocketConnection() = default;

	virtual void upgrade() = 0;
	virtual void do_read() = 0;

  protected:
	explicit WebSocketConnection(const std::shared_ptr<WebSocket>& ws) : ws_(ws)
	{
	}

	const std::shared_ptr<WebSocket> ws_;

	friend class WebSocketServer;
};

void on_session_manage();
void process_received_data(const std::shared_ptr<Session>& session, const std::shared_ptr<Buffer>& buffer);
