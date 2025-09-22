#include "solicare_central_home_hub.hpp"
#include "utils/http_client.hpp"
#include "utils/json_utils.hpp"
#include "utils/jwt_utils.hpp"
#include "utils/logging_utils.hpp"

using namespace std;
using namespace SolicareHomeHub::ApiClient;
using json = nlohmann::json;

bool SolicareCentralHomeHub::process_senior_login(std::string_view user_id, std::string_view password)
{
	try
	{
		const string target = BASE_API_LOGIN_PATH;
		json body_json      = {{"userId", user_id}, {"password", password}};
		string body         = body_json.dump();
		auto res = HttpClient::requestHttps(ioc_, BASE_API_HOST, HttpClient::Method::POST, target, body, 5000);
		if (res.error)
		{
			Logger::log_error(TAG, fmt::format("Login API error: {}", *res.error));
			return false;
		}
		if (!res.body)
		{
			Logger::log_error(TAG, "Login API response body is empty.");
			return false;
		}
		// Logger::log_info(TAG, fmt::format("Login API response: {}", *res.body), Logger::ConsoleColor::WHITE);
		auto res_json_opt = JsonUtils::parse_json(*res.body);
		if (!res_json_opt)
		{
			Logger::log_error(TAG, "JSON parse error.");
			return false;
		}
		const auto& res_json = *res_json_opt;
		if (res_json.contains("message"))
		{
			Logger::log_info(TAG, fmt::format("API Response Message: {}", res_json["message"].get<string>()),
			                 LOG_COLOR);
		}
		if (res.status == 200)
		{
			auto profile_opt = JsonUtils::get_nested_json(res_json, vector<string>{"body", "profile"});
			if (auto token_opt = JsonUtils::get_nested_string(res_json, "body", "token"); profile_opt && token_opt)
			{
				const auto subject_opt = JwtUtils::extractSubjectFromJwt(*token_opt);
				if (!subject_opt)
				{
					Logger::log_error(TAG, "JWT subject extraction failed.");
					return false;
				}
				const auto name = JsonUtils::get_string(*profile_opt, "name");
				if (!name)
				{
					Logger::log_error(TAG, "Profile name extraction failed.");
					return false;
				}
				identity_ = SeniorIdentity(*name, *subject_opt, *token_opt);
				return true;
			}
			Logger::log_error(TAG, "Login response missing body/token or body/profile.");
		}
		// else if (res.status == 401)
		// {
		// 	Logger::log_info(TAG, "Login failed: Unauthorized.", LOG_COLOR);
		// }
		// else
		// {
		// 	Logger::log_info(TAG, fmt::format("Login failed: HTTP status {}", res.status), LOG_COLOR);
		// }
		return false;
	}
	catch (const std::exception& ex)
	{
		Logger::log_error(TAG, fmt::format("Login exception: {}", ex.what()));
	}
	return false;
}