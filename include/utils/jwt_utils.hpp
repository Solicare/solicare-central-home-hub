#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

// Usage Example:
// std::optional<std::string> subject = JwtUtils::extractSubjectFromJwt(token);
// std::optional<std::string> claim = JwtUtils::extractClaimFromJwt(token, "exp");
// if (subject) { /* use subject */ }
// if (claim) { /* use claim value */ }
//
// token: JWT string (header.payload.signature)
// claim: claim name to extract (e.g. "sub", "exp", "aud", ...)
// Returns std::nullopt if extraction fails.
//
// Requires nlohmann::json
namespace JwtUtils
{
// base64url decode (for JWT)
inline std::string base64UrlDecode(const std::string& input)
{
	std::string b64 = input;
	std::ranges::replace(b64, '-', '+');
	std::ranges::replace(b64, '_', '/');
	while (b64.size() % 4)
		b64 += '=';
	static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::vector<unsigned char> out;
	int val = 0, valb = -8;
	for (const unsigned char c : b64)
	{
		if (isspace(c))
			continue;
		if (base64_chars.find(static_cast<char>(c)) == std::string::npos && c != '=')
			return {};
		if (c == '=')
			break;
		val = (val << 6) + static_cast<int>(base64_chars.find(static_cast<char>(c)));
		valb += 6;
		if (valb >= 0)
		{
			out.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
			valb -= 8;
		}
	}
	return {out.begin(), out.end()};
}

inline std::optional<std::string> extractSubjectFromJwt(std::string_view token)
{
	// JWT: header.payload.signature
	const size_t first_dot = token.find('.');
	if (first_dot == std::string_view::npos)
		return std::nullopt;
	const size_t second_dot = token.find('.', first_dot + 1);
	if (second_dot == std::string_view::npos)
		return std::nullopt;
	const auto payload  = std::string(token.substr(first_dot + 1, second_dot - first_dot - 1));
	std::string decoded = base64UrlDecode(payload);
	if (decoded.empty())
		return std::nullopt;
	try
	{
		auto json = nlohmann::json::parse(decoded);
		if (json.contains("sub") && json["sub"].is_string())
		{
			return json["sub"].get<std::string>();
		}
	}
	catch (...)
	{
		return std::nullopt;
	}
	return std::nullopt;
}

// Generic claim extraction function
inline std::optional<std::string> extractClaimFromJwt(std::string_view token, const std::string& claim)
{
	const size_t first_dot = token.find('.');
	if (first_dot == std::string_view::npos)
		return std::nullopt;
	const size_t second_dot = token.find('.', first_dot + 1);
	if (second_dot == std::string_view::npos)
		return std::nullopt;
	const auto payload  = std::string(token.substr(first_dot + 1, second_dot - first_dot - 1));
	std::string decoded = base64UrlDecode(payload);
	if (decoded.empty())
		return std::nullopt;
	try
	{
		if (auto json = nlohmann::json::parse(decoded); json.contains(claim) && json[claim].is_string())
		{
			return json[claim].get<std::string>();
		}
	}
	catch (...)
	{
		return std::nullopt;
	}
	return std::nullopt;
}

} // namespace JwtUtils
