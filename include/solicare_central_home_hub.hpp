#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/lockfree/queue.hpp>
#include <fmt/core.h>
#include <memory>
#include <opencv2/opencv.hpp>
#include <optional>

#include "server/async_websocket_server.hpp"
#include "utils/logging_utils.hpp"

namespace SolicareHomeHub
{
enum MONITOR_MODE
{
	MODE_NORMAL,
	MODE_CAMERA_ONLY,
	MODE_DEVICE_ONLY,
	MODE_OFF
};

enum MONITOR_STATUS
{
	STANDBY,
	STATUS_GOOD,
	STATUS_FALLEN,
	STATUS_NO_HEARTBEAT,
	STATUS_INACTIVITY
};

namespace Launcher
{
inline constexpr std::string_view TAG = "SolicareHomeHub";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::YELLOW;
} // namespace Launcher

namespace SessionManager
{
inline constexpr std::string_view TAG = "SessionManager";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::VIOLET;

enum SESSION_TYPE
{
	SESSION_NOT_IDENTIFIED,
	SESSION_CAMERA,
	SESSION_WEARABLE
};
} // namespace SessionManager

namespace CameraProcessor
{
inline constexpr std::string_view TAG = "CameraProcessor";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::TEAL;

inline constexpr int MAX_BODY_POINT = 30;

enum PersonPosture
{
	UNKNOWN,
	STANDING,
	SITTING,
	LYING,
	FALLEN
};

struct CameraSessionData
{
	std::string device_tag;
	PersonPosture pose = UNKNOWN;
	boost::lockfree::queue<cv::Point2f> body_points{MAX_BODY_POINT};
};
} // namespace CameraProcessor

namespace WearableProcessor
{
inline constexpr std::string_view TAG = "WearableProcessor";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::MAGENTA;
} // namespace WearableProcessor

namespace ApiClient
{
inline constexpr std::string_view TAG = "SolicareApiClient";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::PINK;
} // namespace ApiClient
} // namespace SolicareHomeHub

struct WebSocketServerContext::SessionInfo
{
	SolicareHomeHub::SessionManager::SESSION_TYPE type = SolicareHomeHub::SessionManager::SESSION_NOT_IDENTIFIED;
	std::variant<std::monostate, std::shared_ptr<SolicareHomeHub::CameraProcessor::CameraSessionData>> data{};
	TimePoint timepoint_connected, timepoint_last_received, timepoint_last_processed, timepoint_disconnected;
};

class SolicareCentralHomeHub
{
  public:
	SolicareCentralHomeHub();
	~SolicareCentralHomeHub();

	static void process_image(const std::shared_ptr<WebSocketServerContext::SessionInfo>& session_info,
	                          const std::shared_ptr<WebSocketServerContext::Buffer>& buffer);

	void runtime();

  private:
	boost::asio::io_context ioc_;
	std::thread io_context_run_thread_;
	std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> ioc_work_guard_;
	std::shared_ptr<AsyncWebSocketServer> websocket_server_;

	static int prompt_menu_selection();
	void on_menu_server_start();
	void on_menu_server_stop();
};