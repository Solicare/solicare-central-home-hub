#include "solicare_central_home_hub.hpp"
#include "utils/json_utils.hpp"

using namespace std;
using namespace chrono;
using namespace SolicareHomeHub::WearableProcessor;

void SolicareCentralHomeHub::process_wearable(const std::shared_ptr<WebSocketServerContext::SessionInfo>& session_info,
                                              const std::shared_ptr<WebSocketServerContext::Buffer>& buffer)
{
	try
	{
		const auto& data        = get<shared_ptr<WearableSessionData>>(session_info->data);
		std::string texted_json = boost::beast::buffers_to_string(buffer->data());

		const auto parsed = JsonUtils::parse_json(texted_json);
		if (!parsed)
		{
			Logger::log_error(TAG, fmt::format("JSON parsing failed, Received: {}", texted_json));
			return;
		}
		const auto& j = parsed.value();
		if (j.contains("status"))
			data->is_wearing = (j["status"].get<std::string>() == "ON");
		if (j.contains("fall_detected"))
			data->is_fall_detected = j["fall_detected"].get<bool>();
		if (j.contains("bpm"))
			data->heart_rate_bpm = j["bpm"].get<double>();
		if (j.contains("temperature"))
			data->body_temperature = j["temperature"].get<double>();
		if (j.contains("humidity"))
			data->air_humidity = j["humidity"].get<double>();
		if (j.contains("voltage"))
			data->battery_percentage = j["voltage"].get<double>();

		Logger::log_info(TAG,
		                 fmt::format("[WEARABLE] wear:{}, fall:{}, bpm:{} bpm, temp:{}℃, hum:{}%, volt:{}%",
		                             data->is_wearing, data->is_fall_detected, data->heart_rate_bpm,
		                             data->body_temperature, data->air_humidity, data->battery_percentage),
		                 LOG_COLOR);

		// push SessionData to wearable_data_queue for monitoring (Copy)
		SolicareHomeHub::Monitor::wearable_data_queue.push(*data);
		SolicareHomeHub::Monitor::wearable_last_data_pushed_time = steady_clock::now();
	}
	catch (const std::exception& e)
	{
		Logger::log_error(TAG, fmt::format("Exception in process_wearable: {}", e.what()));
	}
	catch (...)
	{
		Logger::log_error(TAG, "Unknown exception in process_wearable");
	}
}
