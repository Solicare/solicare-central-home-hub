#pragma once

#include "websocket_server_common.h"

class AsyncWebSocketServer final : public WebSocketServer, public std::enable_shared_from_this<AsyncWebSocketServer>
{
public:
	AsyncWebSocketServer(boost::asio::io_context& io_context, unsigned short port);

	class AsyncSocketAcceptor
	{
	public:
		AsyncSocketAcceptor(boost::asio::io_context& io_context, unsigned short port);

		void do_accept();

	private:
		boost::asio::io_context& io_context_;
		std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
		friend class AsyncWebSocketServer;
	};

	void start() override;

private:
	std::shared_ptr<AsyncSocketAcceptor> socket_acceptor_;
	friend class AsyncWebSocketConnection;
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
