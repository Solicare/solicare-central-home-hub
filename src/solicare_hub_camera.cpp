#include <magic_enum.hpp>

#include "solicare_central_home_hub.hpp"
#include "utils/opencv_utils.hpp"

using namespace std;
using namespace chrono;
using namespace boost::beast;
using namespace magic_enum;

using namespace SolicareHomeHub::CameraProcessor;

void SolicareCentralHomeHub::process_image(const shared_ptr<WebSocketServerContext::SessionInfo>& session_info,
                                           const shared_ptr<WebSocketServerContext::Buffer>& buffer)
{
	const auto& data = get<shared_ptr<CameraSessionData>>(session_info->data);

	const cv::Mat bufferedImage(1, static_cast<int>(buffer->size()), CV_8U, buffer->data().data());
	const cv::Mat decoded_image = cv::imdecode(bufferedImage, cv::IMREAD_COLOR);
	if (decoded_image.empty())
	{
		Logger::log_error(TAG,
		                  fmt::format("[Decode] Failed to decode image data from {}: invalid format or corrupted data",
		                              data->device_tag));
		return;
	}

	// TODO: implement pose detection and drawing process here

	const double fps = 1.0 / duration<double>(steady_clock::now() - session_info->timepoint_last_processed).count();
	OpenCVUtils::put_text_overlay(decoded_image, cv::String(enum_name<PersonPosture>(data->pose)),
	                              OpenCVUtils::TEXT_TOP_RIGHT, OpenCVUtils::COLOR_RED);
	OpenCVUtils::put_text_overlay(decoded_image, fmt::format("FPS(Process): {}", static_cast<int>(fps)),
	                              OpenCVUtils::TEXT_TOP_LEFT, OpenCVUtils::COLOR_GREEN);

	cv::imshow(data->device_tag, decoded_image);
	cv::waitKey(1);
}
