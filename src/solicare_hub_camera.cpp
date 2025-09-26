#include <iostream>
#include <magic_enum.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/dnn.hpp>
#include <tbb/concurrent_queue.h>

#include "solicare_central_home_hub.hpp"
#include "utils/opencv_utils.hpp"

using namespace std;
using namespace chrono;
using namespace boost::beast;
using namespace magic_enum;

using namespace SolicareHomeHub::CameraProcessor;

std::optional<cv::dnn::Net> SolicareHomeHub::CameraProcessor::pose_net;

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
	cv::cuda::GpuMat gpu_decoded_image;
	gpu_decoded_image.upload(decoded_image);

	// std::vector<cv::Point2f> keypoints;
	// cv::Mat blob = cv::dnn::blobFromImage(decoded_image, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true,
	// false); pose_net->setInput(blob); cv::Mat output; try
	// {
	// 	output = pose_net->forward();
	// }
	// catch (const cv::Exception& e)
	// {
	// 	std::cerr << "[OpenCV DNN] forward() error: " << e.what() << std::endl;
	// 	Logger::log_error(TAG, fmt::format("[OpenCV DNN] forward() error: {}", e.what()));
	// 	return;
	// }
	// // output: [1, N, 56] (N: 감지된 사람 수, 56: bbox+keypoints)
	// // keypoints: 17개 (x, y, conf)씩
	// if (output.dims == 3)
	// {
	// 	int num = output.size[1];
	// 	for (int i = 0; i < num; ++i)
	// 	{
	// 		const float* row = output.ptr<float>(0, i);
	// 		for (int k = 0; k < 17; ++k)
	// 		{
	// 			float x    = row[6 + k * 3];
	// 			float y    = row[6 + k * 3 + 1];
	// 			float conf = row[6 + k * 3 + 2];
	// 			if (conf > 0.3)
	// 				keypoints.emplace_back(x, y);
	// 		}
	// 		break; // 첫 번째 사람만
	// 	}
	// }
	//
	// // COCO keypoint 연결 순서
	// static const std::vector<std::pair<int, int>> skeleton = {
	//     {0, 1},  {1, 2},   {2, 3},   {3, 4},   // 오른팔
	//     {0, 5},  {5, 6},   {6, 7},   {7, 8},   // 왼팔
	//     {0, 9},  {9, 10},  {10, 11}, {11, 12}, // 오른다리
	//     {0, 13}, {13, 14}, {14, 15}, {15, 16}  // 왼다리
	// };
	// for (const auto& [i, j] : skeleton)
	// {
	// 	if (i < keypoints.size() && j < keypoints.size())
	// 	{
	// 		cv::line(decoded_image, keypoints[i], keypoints[j], cv::Scalar(0, 255, 0), 2);
	// 	}
	// }
	// for (const auto& pt : keypoints)
	// {
	// 	cv::circle(decoded_image, pt, 3, cv::Scalar(0, 0, 255), -1);
	// }

	const double fps = 1.0 / duration<double>(steady_clock::now() - session_info->timepoint_last_processed).count();
	cv::Mat cpu_decoded_image;
	gpu_decoded_image.download(cpu_decoded_image);
	OpenCVUtils::put_text_overlay(cpu_decoded_image, cv::String(enum_name<PersonPosture>(data->pose)),
	                              OpenCVUtils::TEXT_TOP_RIGHT, OpenCVUtils::COLOR_RED);
	OpenCVUtils::put_text_overlay(cpu_decoded_image, fmt::format("FPS(Process): {}", static_cast<int>(fps)),
	                              OpenCVUtils::TEXT_TOP_LEFT, OpenCVUtils::COLOR_GREEN);
	cv::imshow(data->device_tag, cpu_decoded_image);
	cv::waitKey(1);

	// push SessionData to camera_data_queue for monitoring (Copy)
	SolicareHomeHub::Monitor::camera_data_queue.push(*data);
	SolicareHomeHub::Monitor::camera_last_data_pushed_time = steady_clock::now();
}
