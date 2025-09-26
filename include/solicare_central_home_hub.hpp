#pragma once
#include <boost/asio/io_context.hpp>
#include <fmt/core.h>
#include <memory>
#include <opencv2/opencv.hpp>
#include <optional>
#include <tbb/concurrent_queue.h>
#include <thread>

#include "server/async_websocket_server.hpp"
#include "utils/logging_utils.hpp"

namespace SolicareHomeHub
{
inline constexpr auto DEFAULT_WS_SERVER_PORT = 3000;

namespace Launcher
{
inline constexpr std::string_view TAG = "SolicareHomeHub";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::YELLOW;
} // namespace Launcher

namespace ApiClient
{
inline constexpr std::string_view TAG = "SolicareApiClient";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::PINK;

inline constexpr auto BASE_API_HOST       = "dev-api.solicare.kro.kr";
inline constexpr auto BASE_API_PORT       = "443";
inline constexpr auto BASE_API_LOGIN_PATH = "/api/senior/login";

enum API_MONITOR_MODE
{
	FULL_MONITORING,
	CAMERA_ONLY,
	WEARABLE_ONLY,
	NO_MONITORING
};

enum API_MONITOR_EVENT
{
	FALL_DETECTED,
	CAMERA_BATTERY_LOW,
	CAMERA_DISCONNECTED,
	WEARABLE_BATTERY_LOW,
	WEARABLE_DISCONNECTED,
	INACTIVITY_ALERT
};

struct SeniorIdentity
{
	std::string name;
	std::string uuid;
	std::string token;
};
} // namespace ApiClient

namespace SessionManager
{
inline constexpr std::string_view TAG = "SessionManager";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::VIOLET;

enum SESSION_TYPE
{
	SESSION_NOT_IDENTIFIED,
	SESSION_TEST,
	SESSION_CAMERA,
	SESSION_WEARABLE
};
} // namespace SessionManager

namespace CameraProcessor
{
inline constexpr std::string_view TAG = "CameraProcessor";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::TEAL;

inline constexpr int MAX_BODY_POINT          = 30;
inline constexpr auto YOLOV8_POSE_MODEL_PATH = "../models/yolov8n-pose.onnx";

extern std::optional<cv::dnn::Net> pose_net;

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
	std::deque<std::vector<cv::Point2f>> body_points;
};
} // namespace CameraProcessor

namespace WearableProcessor
{
inline constexpr std::string_view TAG = "WearableProcessor";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::BROWN;

struct WearableSessionData
{
	bool is_wearing;
	bool is_fall_detected;
	double heart_rate_bpm;
	double body_temperature;
	double air_humidity;
	double battery_percentage;
};
} // namespace WearableProcessor

namespace Monitor
{
inline constexpr std::string_view TAG = "MonitorProcess";
inline constexpr auto LOG_COLOR       = Logger::ConsoleColor::CYAN;

inline constexpr auto DATA_QUEUE_SIZE = 100;

extern ApiClient::API_MONITOR_MODE mode;

extern std::atomic<std::chrono::steady_clock::time_point> monitor_last_event_time;
extern std::atomic<std::chrono::steady_clock::time_point> monitor_last_processed_time;

extern tbb::concurrent_queue<CameraProcessor::CameraSessionData> camera_data_queue;
extern std::atomic<std::chrono::steady_clock::time_point> camera_last_data_pushed_time;

extern tbb::concurrent_queue<WearableProcessor::WearableSessionData> wearable_data_queue;
extern std::atomic<std::chrono::steady_clock::time_point> wearable_last_worn_time;
extern std::atomic<std::chrono::steady_clock::time_point> wearable_last_data_pushed_time;
} // namespace Monitor

} // namespace SolicareHomeHub

struct WebSocketServerContext::SessionInfo
{
	using SessionType  = SolicareHomeHub::SessionManager::SESSION_TYPE;
	using CameraData   = SolicareHomeHub::CameraProcessor::CameraSessionData;
	using WearableData = SolicareHomeHub::WearableProcessor::WearableSessionData;

	SessionType type = SolicareHomeHub::SessionManager::SESSION_TYPE::SESSION_NOT_IDENTIFIED;
	std::variant<std::monostate, std::string, std::shared_ptr<CameraData>, std::shared_ptr<WearableData>> data{};
	TimePoint timepoint_connected, timepoint_last_received, timepoint_last_processed, timepoint_disconnected;
};

class SolicareCentralHomeHub
{
  public:
	SolicareCentralHomeHub();
	~SolicareCentralHomeHub();
	static void process_image(const std::shared_ptr<WebSocketServerContext::SessionInfo>& session_info,
	                          const std::shared_ptr<WebSocketServerContext::Buffer>& buffer);
	static void process_wearable(const std::shared_ptr<WebSocketServerContext::SessionInfo>& session_info,
	                             const std::shared_ptr<WebSocketServerContext::Buffer>& buffer);
	void login();
	void runtime();
	void start_monitoring();
	void stop_monitoring();

  private:
	boost::asio::io_context ioc_;
	std::thread io_context_run_thread_;
	std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> ioc_work_guard_;

	std::thread monitoring_thread_;
	std::atomic_bool monitoring_active_;

	std::shared_ptr<AsyncWebSocketServer> websocket_server_;

	SolicareHomeHub::ApiClient::SeniorIdentity identity_;

	static int prompt_menu_selection();

	bool process_senior_login(std::string_view user_id, std::string_view password);
	bool fetch_monitoring_status();
	bool postSeniorAlertEvent(const std::string& eventType, const std::string& monitorMode,
	                          const std::string& base64Image);
	bool postSeniorStats(bool cameraFallDetected, bool wearableFallDetected, double temperature, double humidity,
	                     int heartRate, double wearableBattery);
	void on_menu_guardian_mode();
	void on_menu_server_start();
	void on_menu_server_stop();
};