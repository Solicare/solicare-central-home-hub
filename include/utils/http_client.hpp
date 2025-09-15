#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <optional>
#include <string>
#include <unordered_map>

// Usage Example:
// auto res = HttpClient::requestHttp(ioc, host, Method::GET, "/api", "", 3000);
// if (res.error) { /* error handling */ }
// if (res.body) { /* use response body */ }
//
// requestHttp: HTTP request (GET/POST/PUT/DELETE)
// requestHttpWithAuth: HTTP request with Authorization header
// requestHttps: HTTPS request (GET/POST/PUT/DELETE)
// requestHttpsWithAuth: HTTPS request with Authorization header
// All functions support timeout (ms)
//
// Returns std::nullopt on error, otherwise HttpsResponse
//
// Requires boost::asio, boost::beast
namespace HttpClient
{
namespace http = boost::beast::http;
using tcp      = boost::asio::ip::tcp;

enum class Method
{
	GET,
	POST,
	PUT,
	DELETE
};

struct HttpResponse
{
	int status                       = -1;
	std::optional<std::string> body  = std::nullopt;
	std::optional<std::string> error = std::nullopt;
};

namespace HttpClientImpl
{
inline HttpResponse requestImpl(boost::asio::io_context& ioc, const std::string& host, Method method,
                                const std::string& uri_path, const std::string& body, bool use_ssl,
                                const std::string& auth_token, int timeout_ms)
{
	http::verb beast_method;
	switch (method)
	{
	case Method::GET:
		beast_method = http::verb::get;
		break;
	case Method::POST:
		beast_method = http::verb::post;
		break;
	case Method::PUT:
		beast_method = http::verb::put;
		break;
	case Method::DELETE:
		beast_method = http::verb::delete_;
		break;
	default:
		return HttpResponse{-1, std::nullopt, std::make_optional<std::string>("Invalid HTTP method")};
	}
	try
	{
		std::string port = use_ssl ? "443" : "80";
		tcp::resolver resolver(ioc);
		auto const results                                           = resolver.resolve(host, port);
		std::unordered_map<std::string, std::string> default_headers = {{"Content-Type", "application/json"}};
		if (!auth_token.empty())
		{
			default_headers["Authorization"] = "Bearer " + auth_token;
		}
		if (use_ssl)
		{
			boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::sslv23_client);
			ssl_ctx.set_default_verify_paths();
			boost::beast::ssl_stream<tcp::socket> stream(ioc, ssl_ctx);
			boost::asio::connect(stream.next_layer(), results);
			if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
			{
				return HttpResponse{-1, std::nullopt,
				                    std::make_optional<std::string>("SSL_set_tlsext_host_name failed")};
			}
			stream.handshake(boost::asio::ssl::stream_base::client);
			http::request<http::string_body> req{beast_method, uri_path, 11};
			req.set(http::field::host, host);
			for (const auto& [fst, snd] : default_headers)
				req.set(fst, snd);
			if (!body.empty())
				req.body() = body;
			req.prepare_payload();
			boost::asio::deadline_timer timer(ioc);
			timer.expires_from_now(boost::posix_time::milliseconds(timeout_ms));
			http::write(stream, req);
			boost::beast::flat_buffer buffer;
			http::response<http::string_body> res;
			http::read(stream, buffer, res);
			boost::system::error_code ec;
			stream.shutdown(ec);
			if (ec)
			{
				return HttpResponse{static_cast<int>(res.result_int()), std::make_optional(res.body()),
				                    std::make_optional(ec.message())};
			}
			return HttpResponse{static_cast<int>(res.result_int()), std::make_optional(res.body()), std::nullopt};
		}

		tcp::socket socket(ioc);
		boost::asio::connect(socket, results);
		http::request<http::string_body> req{beast_method, uri_path, 11};
		req.set(http::field::host, host);
		for (const auto& [fst, snd] : default_headers)
			req.set(fst, snd);
		if (!body.empty())
			req.body() = body;
		req.prepare_payload();
		boost::asio::deadline_timer timer(ioc);
		timer.expires_from_now(boost::posix_time::milliseconds(timeout_ms));
		http::write(socket, req);
		boost::beast::flat_buffer buffer;
		http::response<http::string_body> res;
		http::read(socket, buffer, res);
		boost::system::error_code ec;
		socket.shutdown(tcp::socket::shutdown_both, ec);
		if (ec)
		{
			return HttpResponse{static_cast<int>(res.result_int()), std::make_optional(res.body()),
			                    std::make_optional(ec.message())};
		}
		return HttpResponse{static_cast<int>(res.result_int()), std::make_optional(res.body()), std::nullopt};
	}
	catch (const std::exception& e)
	{
		return HttpResponse{-1, std::nullopt, std::make_optional<std::string>(e.what())};
	}
	catch (...)
	{
		return HttpResponse{-1, std::nullopt, std::make_optional<std::string>("Unknown error")};
	}
}
} // namespace HttpClientImpl

inline HttpResponse requestHttp(boost::asio::io_context& ioc, const std::string& host, const Method method,
                                const std::string& uri_path, const std::string& body, const int timeout_ms = 5000)
{
	return HttpClientImpl::requestImpl(ioc, host, method, uri_path, body, false, "", timeout_ms);
}

inline HttpResponse requestHttpWithAuth(boost::asio::io_context& ioc, const std::string& auth_token,
                                        const std::string& host, const Method method, const std::string& uri_path,
                                        const std::string& body, const int timeout_ms = 5000)
{
	return HttpClientImpl::requestImpl(ioc, host, method, uri_path, body, false, auth_token, timeout_ms);
}

inline HttpResponse requestHttps(boost::asio::io_context& ioc, const std::string& host, const Method method,
                                 const std::string& uri_path, const std::string& body, const int timeout_ms = 5000)
{
	return HttpClientImpl::requestImpl(ioc, host, method, uri_path, body, true, "", timeout_ms);
}

inline HttpResponse requestHttpsWithAuth(boost::asio::io_context& ioc, const std::string& auth_token,
                                         const std::string& host, const Method method, const std::string& uri_path,
                                         const std::string& body, const int timeout_ms = 5000)
{
	return HttpClientImpl::requestImpl(ioc, host, method, uri_path, body, true, auth_token, timeout_ms);
}
} // namespace HttpClient
