#include "solicare_central_home_hub.hpp"

using namespace std;
using namespace chrono;
using namespace boost::beast;

using namespace WebSocketServerContext;
using namespace SolicareHomeHub;
using namespace SolicareHomeHub::SessionManager;

void WebSocketServerContext::on_session_created(const shared_ptr<Session>& session, const shared_ptr<Buffer>& buffer)
{
	const auto device_ip = session->ws->next_layer().remote_endpoint().address().to_string();
	if (!session->ws->got_text())
	{
		Logger::log_warn(
		    TAG, fmt::format("Expected text message to identify device type, but received other {}", device_ip));
		return;
	}

	const auto message = buffers_to_string(buffer->data());
	Logger::log_info(TAG, fmt::format("[Created] New session: device_ip={} | message={}", device_ip, message),
	                 LOG_COLOR);

	// TODO: send same message back to the device to confirm connection established
	// ws->text(true);
	// ws->async_write(buffer->data(),
	//                 [](const boost::system::error_code& ec, std::size_t bytes_transferred)
	//                 {
	//                     if (ec)
	//                     {
	//                         error(TAG, fmt::format("[Session] Error sending confirmation message: {}",
	//                         ec.message()));
	//                     }
	//                 });

	session->info                           = make_shared<SessionInfo>();
	session->info->timepoint_connected      = steady_clock::now();
	session->info->timepoint_last_received  = steady_clock::now();
	session->info->timepoint_last_processed = steady_clock::now();
	if (message.find("CAM") != string::npos)
	{
		session->info->type     = SESSION_CAMERA;
		session->info->data     = make_shared<CameraProcessor::CameraSessionData>();
		const auto camera_data  = get<shared_ptr<CameraProcessor::CameraSessionData>>(session->info->data);
		camera_data->device_tag = fmt::format("{}({})", message, device_ip);
	}
	else
	{
		Logger::log_warn(TAG, fmt::format("Unknown device type received: '{}' from {}", message, device_ip));
	}
}

void WebSocketServerContext::on_session_read(const shared_ptr<Session>& session, const shared_ptr<Buffer>& buffer)
{
	session->info->timepoint_last_received = steady_clock::now();
	if (session->info->type == SESSION_CAMERA &&
	    holds_alternative<shared_ptr<CameraProcessor::CameraSessionData>>(session->info->data))
	{
		SolicareCentralHomeHub::process_image(session->info, buffer);
	}
	else
	{
		const auto device_ip = session->ws->next_layer().remote_endpoint().address().to_string();
		Logger::log_warn(
		    TAG, fmt::format("Received data for unsupported session type or uninitialized session from {}", device_ip));
		return;
	}
	session->info->timepoint_last_processed = steady_clock::now();
}

void WebSocketServerContext::on_session_manage()
{
	if (!ws_session_map.empty())
	{
		vector<string> sessions_to_remove;
		sessions_to_remove.reserve(ws_session_map.size());
		ws_session_map.cvisit_all(
		    [&](const auto& pair)
		    {
			    const auto& device_ip = pair.first;
			    const auto& session   = pair.second;

			    if (!session || !session->info)
			    {
				    sessions_to_remove.push_back(device_ip);
				    return;
			    }

			    const auto now = steady_clock::now();
			    const auto time_since_connected =
			        duration_cast<seconds>(now - session->info->timepoint_connected).count();
			    const auto time_since_last_received =
			        duration_cast<seconds>(now - session->info->timepoint_last_received).count();

			    if (session->ws && session->ws->is_open() && session->ws->next_layer().is_open())
			    {
				    if (time_since_connected > ws_server_config.session_timeout &&
				        time_since_last_received > ws_server_config.session_timeout)
				    {
					    Logger::log_warn(
					        TAG, fmt::format(
					                 "[Timeout] session '{}' will be disconnected by timeout (last received: {}s ago)",
					                 device_ip, time_since_last_received));
					    session->ws->async_close(
					        websocket::close_code::normal,
					        [device_ip](const boost::system::error_code& ec)
					        {
						        if (ec)
						        {
							        Logger::log_error(TAG, fmt::format("Error closing websocket for session '{}': {}",
							                                           device_ip, ec.message()));
						        }
					        });
					    sessions_to_remove.push_back(device_ip);
				    }
			    }
			    else
			    {
				    log_info(TAG, fmt::format("[Disconnect] session '{}' had been disconnect.", device_ip), LOG_COLOR);
				    sessions_to_remove.push_back(device_ip);
			    }
		    });

		for (const auto& device_ip : sessions_to_remove)
		{
			Logger::log_info(TAG, fmt::format("[Removed] session '{}' had been removed.", device_ip), LOG_COLOR);
			ws_session_map.erase(device_ip);
		}
	}

	if (const auto now = steady_clock::now(); duration_cast<seconds>(now - ws_last_session_logged).count() >= 10)
	{
		Logger::log_info(TAG, fmt::format("active sessions: {}", ws_session_map.size()), Logger::ConsoleColor::WHITE);
		ws_last_session_logged = now;
	}
}
