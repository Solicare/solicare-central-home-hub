#include <boost/beast/core/buffers_to_string.hpp>
#include <iostream>
#include <magic_enum.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include "server/async_websocket_server.h"
#include "solicare_central_hub_server.h"
#include "utils/opencv_utils.h"

using namespace std;
using namespace chrono;
using namespace boost::beast;

Session::Session(const shared_ptr<WebSocket>& websocket, const shared_ptr<Buffer>& buffer) : ws(websocket)
{
	const auto device_ip = websocket->next_layer().remote_endpoint().address().to_string();
	if (!websocket->got_text())
	{
		cerr << format(
				"[Session][Error] Expected text message to identify device type, but received binary data from {}",
				device_ip)
			<< endl;
		return;
	}
	const auto message = buffers_to_string(buffer->data());

	info                      = make_shared<SessionInfo>();
	info->timepoint_connected = steady_clock::now();
	if (message.find("CAM") != string::npos)
	{
		info->type             = SESSION_CAMERA;
		info->data             = make_shared<CameraData>();
		const auto camera_data = get<shared_ptr<CameraData>>(info->data);
		camera_data->tag       = format("{}({})", message, device_ip);
	}
	else
	{
		cerr << format("[Session][Warning] Unknown device type received: '{}' from {}", message, device_ip) << endl;
	}
}

void process_image(const shared_ptr<SessionInfo>& session_info, const shared_ptr<Buffer>& buffer)
{
	const auto& data = get<shared_ptr<CameraData>>(session_info->data);

	const cv::Mat bufferedImage(1, static_cast<int>(buffer->size()), CV_8U, buffer->data().data());
	const cv::Mat decoded_image = cv::imdecode(bufferedImage, cv::IMREAD_COLOR);
	if (decoded_image.empty())
	{
		cerr << format("[Session][{}][DecodeError]: Failed to decode image - invalid format or corrupted data",
		               data->tag)
			<< endl;
		return;
	}

	// TODO: implement pose detection and drawing process here

	const double fps = 1.0 / duration<double>(steady_clock::now() - session_info->timepoint_last_processed).count();
	put_text_overlay(decoded_image, cv::String(magic_enum::enum_name(data->status)), TEXT_TOP_RIGHT, COLOR_RED);
	put_text_overlay(decoded_image, format("FPS(Process): {}", to_string(static_cast<int>(fps))), TEXT_TOP_LEFT,
	                 COLOR_GREEN);

	cv::imshow(data->tag, decoded_image);
	cv::waitKey(1);
}

void process_received_data(const shared_ptr<Session>& session, const shared_ptr<Buffer>& buffer)
{
	session->info->timepoint_last_received = steady_clock::now();
	if (session->info->type == SESSION_CAMERA && holds_alternative<shared_ptr<CameraData>>(session->info->data))
	{
		process_image(session->info, buffer);
	}
	else
	{
		const auto device_ip = session->ws->next_layer().remote_endpoint().address().to_string();
		cerr << format("[Session][{}][Error] Received data for unsupported session type or uninitialized session data "
		               "structure",
		               device_ip)
			<< endl;
		return;
	}
	session->info->timepoint_last_processed = steady_clock::now();
}

void on_session_manage()
{
	if (!sessions.empty())
	{
		vector<string> sessions_to_remove;
		sessions_to_remove.reserve(sessions.size());
		sessions.cvisit_all(
			[&](const auto& pair)
			{
				const auto& device_ip = pair.first;
				const auto& session   = pair.second;

				if (!session || !session->info)
				{
					sessions_to_remove.push_back(device_ip);
					return;
				}

				const auto now                  = steady_clock::now();
				const auto time_since_connected =
					duration_cast<seconds>(now - session->info->timepoint_connected).count();
				const auto time_since_last_received =
					duration_cast<seconds>(now - session->info->timepoint_last_received).count();

				if (time_since_connected > 2 && time_since_last_received > SESSION_TIMEOUT)
				{
					// if (session->info->type == SESSION_CAMERA &&
					//     holds_alternative<shared_ptr<CameraData>>(session->info->data))
					// {
					//     const auto camera_data = get<shared_ptr<CameraData>>(session->info->data);
					//     try
					//     {
					//         cv::destroyWindow(camera_data->tag); // TODO: solve the issue with destroying windows
					//         (only main thread can destory windows)
					//     }
					//     catch (const exception& e)
					//     {
					//         cerr << format("[Session][OpenCV][Error] Failed to close window for {}: {}",
					//                        camera_data->tag, e.what()) << endl;
					//     }
					// }
					cerr << format("[Session][Timeout] session '{}' will be removed (last received: {}s ago)",
					               device_ip, time_since_last_received)
						<< endl;
					sessions_to_remove.push_back(device_ip);
				}
			});

		for (const auto& device_ip : sessions_to_remove)
		{
			sessions.erase(device_ip);
		}
	}

	static auto last_status_log = steady_clock::now();
	if (const auto now = steady_clock::now(); duration_cast<seconds>(now - last_status_log).count() >= 10)
	{
		cout << format("[Session] active sessions: {}", sessions.size()) << endl;
		last_status_log = now;
	}
}