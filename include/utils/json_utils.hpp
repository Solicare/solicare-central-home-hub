#pragma once
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// Usage Example:
// JsonUtils::log_enabled = true; // Enable logging (default: false)
// auto j = JsonUtils::parse_json(json_str);
// auto name = JsonUtils::get_string(j.value(), "name");
// auto nested = JsonUtils::get_nested_string(j.value(), "user", "id");
// if (name) { /* use name */ }
// if (nested) { /* use nested value */ }
//
// parse_json: Parses JSON string, returns std::nullopt on error.
// get_string: Extracts a string field from JSON object.
// get_nested_string: Extracts a string field from a nested JSON object.
// If log_enabled is true, errors are printed to std::cerr.
//
// Requires nlohmann::json
namespace JsonUtils
{
using json = nlohmann::json;

inline bool log_enabled = false;

// Parse a JSON string. If parsing fails, return std::nullopt. If log_enabled is true, print error to cerr.
inline std::optional<json> parse_json(const std::string& body)
{
	try
	{
		return json::parse(body);
	}
	catch (const std::exception& e)
	{
		if (log_enabled)
			std::cerr << "Json parse error: " << e.what() << std::endl;
		return std::nullopt;
	}
	catch (...)
	{
		if (log_enabled)
			std::cerr << "Json parse error: unknown error" << std::endl;
		return std::nullopt;
	}
}

// Safely extract a string field from a JSON object.
inline std::optional<std::string> get_string(const json& j, const std::string& field)
{
	if (j.contains(field) && j[field].is_string())
	{
		return j[field].get<std::string>();
	}
	return std::nullopt;
}

// Safely extract a string field from a nested JSON object.
inline std::optional<std::string> get_nested_string(const json& j, const std::string& obj, const std::string& field)
{
	if (j.contains(obj) && j[obj].contains(field) && j[obj][field].is_string())
	{
		return j[obj][field].get<std::string>();
	}
	return std::nullopt;
}
} // namespace JsonUtils