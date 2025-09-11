#pragma once
#include "websocket_server_common.hpp"

class AsyncSocketAcceptor;

class AsyncWebSocketServer final : public WebSocketServer, public std::enable_shared_from_this<AsyncWebSocketServer>
{
  public:
	explicit AsyncWebSocketServer(boost::asio::io_context& io_context);

	void start() override;
	void stop() override;

  private:
	std::shared_ptr<AsyncSocketAcceptor> socket_acceptor_;
	friend class AsyncWebSocketConnection;
};

class AsyncSocketAcceptor
{
  public:
	explicit AsyncSocketAcceptor(boost::asio::io_context& io_context);

	void do_accept();

  private:
	boost::asio::io_context& io_context_;
	std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
	friend class AsyncWebSocketServer;
};

class AsyncWebSocketConnection final : public WebSocketConnection,
                                       public std::enable_shared_from_this<AsyncWebSocketConnection>
{
  public:
	explicit AsyncWebSocketConnection(const std::shared_ptr<Socket>& socket);

	void upgrade() override;
	void do_read() override;

  private:
	friend class AsyncWebSocketServer;
};
