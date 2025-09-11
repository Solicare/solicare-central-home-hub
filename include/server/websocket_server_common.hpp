#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <fmt/core.h>
#include <memory>

using Socket = boost::asio::ip::tcp::socket;

namespace WebSocketServerContext
{
// -- Types --
struct Session;
struct SessionInfo;

// Type aliases for convenience
using Duration   = std::chrono::steady_clock::duration;
using TimePoint  = std::chrono::steady_clock::time_point;
using Buffer     = boost::beast::flat_buffer;
using WebSocket  = boost::beast::websocket::stream<Socket>;
using SessionMap = boost::concurrent_flat_map<std::string, std::shared_ptr<Session>>;

// Server configuration structure
struct ServerConfig
{
	int server_port              = 3000;
	int max_session              = 5;
	int session_timeout          = 5;
	int session_manage_period_ms = 50;
};

// Session structure holding WebSocket and session info
struct Session
{
	std::shared_ptr<WebSocket> ws;
	std::shared_ptr<SessionInfo> info;
};

// -- Global State --
extern bool flag_stop_server;            // Server Stop Flag for logging
extern ServerConfig ws_server_config;    // Server configuration
extern SessionMap ws_session_map;        // Session map
extern TimePoint ws_last_session_logged; // Last time a session logged

// -- Session Event Handlers --
void on_session_created(const std::shared_ptr<Session>& session,
                        const std::shared_ptr<Buffer>& buffer); // Called when a session is created
void on_session_read(const std::shared_ptr<Session>& session,
                     const std::shared_ptr<Buffer>& buffer); // Called when a session receives data

// -- Session Management --
void on_session_manage(); // Periodic session management (timeout, cleanup, etc.)
} // namespace WebSocketServerContext

class WebSocketServer
{
  public:
	virtual ~WebSocketServer() = default;
	virtual void start()       = 0;
	virtual void stop()        = 0;

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
	explicit WebSocketConnection(const std::shared_ptr<WebSocketServerContext::WebSocket>& ws) : ws_(ws)
	{
	}

	const std::shared_ptr<WebSocketServerContext::WebSocket> ws_;

	friend class WebSocketServer;
};