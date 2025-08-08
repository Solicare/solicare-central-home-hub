#pragma once
#include <boost/lockfree/queue.hpp>
#include <opencv2/opencv.hpp>

#include "server/websocket_server_common.h"

#define MAX_BODY_POINT 30

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

enum POSE_STATUS
{
	NONE,
	DETECTED,
	ANALYZING,
	STANDING,
	SITTING,
	LYING,
	FALLEN
};

enum SESSION_TYPE
{
	SESSION_NOT_IDENTIFIED,
	SESSION_CAMERA,
	SESSION_WEARABLE
};

struct CameraData
{
	std::string tag    = "Unknown";
	POSE_STATUS status = NONE;
	boost::lockfree::queue<cv::Point2f> body_points{MAX_BODY_POINT};
};

struct SessionInfo
{
	SESSION_TYPE type = SESSION_NOT_IDENTIFIED;
	std::variant<std::monostate, std::shared_ptr<CameraData>> data{};
	TimePoint timepoint_connected, timepoint_last_received, timepoint_last_processed, timepoint_disconnected;
};