#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>

#include "include/server/async_websocket_server.h"

using namespace std;
using namespace boost::asio;

void start_async_server(int port)
{
	try
	{
		io_context io_context;
		const auto server = make_shared<AsyncWebSocketServer>(io_context, port);
		server->start();
		io_context.run();
	}
	catch (const boost::system::system_error& e)
	{
		cerr << "Network error in async server: " << e.what() << endl;
		throw;
	}
	catch (const exception& e)
	{
		cerr << "Error starting or during async server operation: " << e.what() << endl;
		throw;
	}
}

void print_usage(const char* program_name)
{
	cout << "Solicare Central Hub - WebSocket server for real-time home monitoring and health tracking" << endl;
	cout << "Usage: " << program_name << " <port>" << endl;
	cout << endl;
	cout << "Parameters:" << endl;
	cout << "  port          Port number to listen on (1-65535)" << endl;
	cout << endl;
	cout << "Examples:" << endl;
	cout << "  " << program_name << " async 3000    Start async server on port 3000" << endl;
}

int main(const int argc, char** argv)
{
	if (argc < 2)
	{
		print_usage(argv[0]);
		return 1;
	}

	int port;
	try
	{
		if (port = stoi(argv[1]); port <= 0 || port > 65535)
		{
			cerr << "Error: Port number must be between 1 and 65535" << endl;
			print_usage(argv[0]);
			return 1;
		}
	}
	catch (const exception& e)
	{
		cerr << "Error: Invalid port number" << endl;
		print_usage(argv[0]);
		return 1;
	}

	cout << "Starting async server on port " << port << endl;
	start_async_server(port);
	return 0;
}
