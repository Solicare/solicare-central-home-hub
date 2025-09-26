#include "solicare_central_home_hub.hpp"
#include "utils/system_utils.hpp"
#include <chrono>
#include <tbb/concurrent_queue.h>
#include <thread>

using namespace std;
using namespace std::chrono;

using namespace Logger;

using namespace SolicareHomeHub::Monitor;

namespace SolicareHomeHub::Monitor
{
ApiClient::API_MONITOR_MODE mode = ApiClient::FULL_MONITORING;
tbb::concurrent_queue<CameraProcessor::CameraSessionData> camera_data_queue =
    tbb::concurrent_queue<CameraProcessor::CameraSessionData>();
tbb::concurrent_queue<WearableProcessor::WearableSessionData> wearable_data_queue =
    tbb::concurrent_queue<WearableProcessor::WearableSessionData>();
std::atomic<steady_clock::time_point> monitor_last_event_time        = steady_clock::now();
std::atomic<steady_clock::time_point> monitor_last_processed_time    = steady_clock::now();
std::atomic<steady_clock::time_point> camera_last_data_pushed_time   = steady_clock::now();
std::atomic<steady_clock::time_point> wearable_last_worn_time        = steady_clock::now();
std::atomic<steady_clock::time_point> wearable_last_data_pushed_time = steady_clock::now();
} // namespace SolicareHomeHub::Monitor

// 유틸 함수: enum을 string으로 변환
static std::string monitorEventToString(const SolicareHomeHub::ApiClient::API_MONITOR_EVENT event)
{
	using namespace SolicareHomeHub::ApiClient;
	switch (event)
	{
	case FALL_DETECTED:
		return "FALL_DETECTED";
	case CAMERA_DISCONNECTED:
		return "CAMERA_DISCONNECTED";
	case WEARABLE_DISCONNECTED:
		return "WEARABLE_DISCONNECTED";
	default:
		return "";
	}
}

static std::string monitorModeToString(const SolicareHomeHub::ApiClient::API_MONITOR_MODE mode)
{
	using namespace SolicareHomeHub::ApiClient;
	switch (mode)
	{
	case FULL_MONITORING:
		return "FULL_MONITORING";
	case CAMERA_ONLY:
		return "CAMERA_ONLY";
	case WEARABLE_ONLY:
		return "WEARABLE_ONLY";
	case NO_MONITORING:
		return "NO_MONITORING";
	default:
		return "UNKNOWN";
	}
}

void SolicareCentralHomeHub::on_menu_guardian_mode()
{
	log_info(TAG, "보호자 제어 모드가 활성화 되었습니다. 종료하시려면 q키를 누르세요.\n", LOG_COLOR);

	std::atomic guardian_monitoring_active{true};
	std::thread guardian_monitor_thread(
	    [this, &guardian_monitoring_active]()
	    {
		    bool prev_state = false;
		    while (guardian_monitoring_active)
		    {
			    // log_info(TAG, fmt::format("모니터링 상태 수신: {}", monitoring ? "true" : "false"), LOG_COLOR);
			    if (const bool monitoring = fetch_monitoring_status(); prev_state != monitoring)
			    {
				    log_warn(TAG, fmt::format("보호자에 의해 모니터링 상태가 변경되었습니다: {} → {}",
				                              prev_state ? "활성화" : "비활성화", monitoring ? "활성화" : "비활성화"));
				    if (monitoring == true && !websocket_server_)
				    {
					    log_info(TAG,
					             fmt::format("보호자 모니터링 활성화 → 비동기 웹소켓 서버를 {}번 포트에서 시작합니다.",
					                         SolicareHomeHub::DEFAULT_WS_SERVER_PORT),
					             LOG_COLOR);
					    WebSocketServerContext::flag_stop_server             = false;
					    WebSocketServerContext::ws_server_config.server_port = SolicareHomeHub::DEFAULT_WS_SERVER_PORT;
					    WebSocketServerContext::ws_session_map.clear();
					    websocket_server_ = make_shared<AsyncWebSocketServer>(ioc_);
					    websocket_server_->start();
					    mode = SolicareHomeHub::ApiClient::FULL_MONITORING;
					    start_monitoring();
				    }
				    if (monitoring == false && websocket_server_)
				    {
					    log_info(TAG, "보호자 모니터링 중단 → 실행중인 비동기 웹소켓 서버를 중지합니다.", LOG_COLOR);
					    stop_monitoring();
					    websocket_server_->stop();
					    this_thread::sleep_for(milliseconds(1000));
					    websocket_server_.reset();
				    }
				    prev_state = monitoring;
			    }
			    std::this_thread::sleep_for(milliseconds(1000));
		    }
		    cout << std::endl;
		    log_info(TAG, "보호자 제어 모드를 종료합니다.", LOG_COLOR);
		    if (websocket_server_)
		    {
			    stop_monitoring();
			    websocket_server_->stop();
			    this_thread::sleep_for(milliseconds(1000));
			    websocket_server_.reset();
		    }
	    });
	while (true)
	{
		if (const char key = InputUtils::get_key_press(); key == 'q' || key == 'Q')
		{
			guardian_monitoring_active = false;
			break;
		}
		log_info(TAG, "보호자 제어 모드가 활성화 되었습니다. 종료하시려면 q키를 누르세요.\n", LOG_COLOR);
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	if (guardian_monitor_thread.joinable())
		guardian_monitor_thread.join();
}

void SolicareCentralHomeHub::start_monitoring()
{
	if (monitoring_active_)
		return;

	steady_clock::time_point start_time                        = steady_clock::now();
	SolicareHomeHub::ApiClient::API_MONITOR_MODE previous_mode = SolicareHomeHub::ApiClient::FULL_MONITORING;
	monitoring_active_                                         = true;
	monitoring_thread_                                         = std::thread(
        [this, &previous_mode, start_time]()
        {
            log_info(TAG, "Solicare 시니어 케어 모니터링 서비스를 시작합니다.", LOG_COLOR);
            using namespace std::chrono_literals;
            while (true)
            {
                if (!monitoring_active_)
                    break;

                auto now = steady_clock::now();
                if (duration_cast<seconds>(now - start_time).count() < TIME_TO_WAIT_DATA)
                {
                    // 10초 이내면 분리 감지 및 모든 처리를 건너뜀
                    std::this_thread::sleep_for(1s);
                    continue;
                }

                auto camera_gap    = duration_cast<seconds>(now - camera_last_data_pushed_time.load()).count();
                auto wearable_gap  = duration_cast<seconds>(now - wearable_last_data_pushed_time.load()).count();
                auto last_wear_gap = duration_cast<seconds>(now - wearable_last_worn_time.load()).count();

                // 복합 분리 감지 조건
                bool camera_detached   = camera_gap > TIME_TO_WAIT_DATA;
                bool wearable_detached = (wearable_gap > TIME_TO_WAIT_DATA) || (last_wear_gap > TIME_TO_WAIT_DATA);

                // 테스트용 한 줄 로그 (기존 로그는 생략)
                log_info(TAG, fmt::format("[TEST] camera_detached: {} | wearable_detached: {} | camera_gap: {}s | "
			                                                                                                              "wearable_gap: {}s | last_wear_gap:	{}s ",
			                                                                      camera_detached, wearable_detached, camera_gap, wearable_gap, last_wear_gap));

                // mode 결정 (둘 다 분리면 웨어러블만 분리로 간주)
                if (camera_detached && wearable_detached)
                {
                    mode = SolicareHomeHub::ApiClient::WEARABLE_ONLY;
                }
                else if (camera_detached)
                {
                    mode = SolicareHomeHub::ApiClient::WEARABLE_ONLY;
                }
                else if (wearable_detached)
                {
                    mode = SolicareHomeHub::ApiClient::CAMERA_ONLY;
                }
                else
                {
                    mode = SolicareHomeHub::ApiClient::FULL_MONITORING;
                }

                // 알림 로직: 분리 이벤트만, 복구는 무시. 같은 분리 이벤트가 60초 이내 반복되면 로그만 남김
                static bool prev_camera_detached                          = false;
                static bool prev_wearable_detached                        = false;
                static steady_clock::time_point last_camera_notify_time   = steady_clock::now() - minutes(1);
                static steady_clock::time_point last_wearable_notify_time = steady_clock::now() - minutes(1);

                // 카메라 분리 이벤트(복구는 무시)
                if (camera_detached && camera_detached != prev_camera_detached)
                {
                    auto now_tp = steady_clock::now();
                    if (auto since_last_notify = duration_cast<seconds>(now_tp - last_camera_notify_time).count();
                        since_last_notify >= 60)
                    {
                        log_warn(TAG, "[EVENT] 카메라 디바이스가 분리되었습니다. 보호자에게 알림을 전송합니다.");
                        postSeniorAlertEvent(monitorEventToString(SolicareHomeHub::ApiClient::CAMERA_DISCONNECTED),
					                                                                 monitorModeToString(mode), "");
                        last_camera_notify_time = now_tp;
                    }
                    else
                    {
                        log_info(TAG,
					                                                     fmt::format("[LOG] 카메라 분리 감지({}초 이내, {}초 경과). API 호출 생략.", 60,
					                                                                 since_last_notify),
					                                                     LOG_COLOR);
                    }
                }

                // 웨어러블 분리 이벤트(복구는 무시)
                if (wearable_detached && wearable_detached != prev_wearable_detached)
                {
                    auto now_tp = steady_clock::now();
                    if (auto since_last_notify = duration_cast<seconds>(now_tp - last_wearable_notify_time).count();
                        since_last_notify >= 60)
                    {
                        log_warn(TAG, "[EVENT] 웨어러블 디바이스가 분리되었습니다. 보호자에게 알림을 전송합니다.");
                        postSeniorAlertEvent(monitorEventToString(SolicareHomeHub::ApiClient::WEARABLE_DISCONNECTED),
					                                                                 monitorModeToString(mode), "");
                        last_wearable_notify_time = now_tp;
                    }
                    else
                    {
                        log_info(TAG,
					                                                     fmt::format("[LOG] 웨어러블 분리 감지({}초 이내, {}초 경과). API 호출 생략.", 60,
					                                                                 since_last_notify),
					                                                     LOG_COLOR);
                    }
                }
                // 이전 상태 갱신
                prev_camera_detached   = camera_detached;
                prev_wearable_detached = wearable_detached;

                int camera_data_count       = 0;
                bool camera_fallen_detected = false, wearable_fallen_detected = false;
                SolicareHomeHub::CameraProcessor::CameraSessionData img_data;
                while (camera_data_queue.try_pop(img_data))
                {
                    camera_data_count += 1;
                    if (img_data.pose == SolicareHomeHub::CameraProcessor::PersonPosture::FALLEN)
                    {
                        camera_fallen_detected = true;
                    }
                }
                double wearable_data_count = 0, temperature_sum = 0.0, humidity_sum = 0.0, bpm_sum = 0.0,
                       battery_sum = 0.0;
                SolicareHomeHub::WearableProcessor::WearableSessionData wearable_data;
                while (wearable_data_queue.try_pop(wearable_data))
                {
                    wearable_data_count += 1;
                    temperature_sum += wearable_data.body_temperature;
                    humidity_sum += wearable_data.air_humidity;
                    bpm_sum += wearable_data.heart_rate_bpm;
                    battery_sum += wearable_data.battery_percentage;

                    if (wearable_data.is_fall_detected == true)
                    {
                        wearable_fallen_detected = true;
                    }
                    // 착용 중이면 마지막 착용 시각 갱신
                    if (wearable_data.is_wearing == true)
                    {
                        wearable_last_worn_time = steady_clock::now();
                    }
                }

                // 평균값 계산 (0으로 나누는 것 방지)
                const double temperature_avg = wearable_data_count > 0 ? (temperature_sum / wearable_data_count) : 0.0;
                const double humidity_avg    = wearable_data_count > 0 ? (humidity_sum / wearable_data_count) : 0.0;
                const double bpm_avg         = wearable_data_count > 0 ? (bpm_sum / wearable_data_count) : 0.0;
                const double battery_avg     = wearable_data_count > 0 ? (battery_sum / wearable_data_count) : 0.0;

                // TODO: 카메라 디바이스 배터리 검사 + API호출

                if (battery_avg <= 30)
                {
                    // TODO: API호출(웨어러블 배터리 부족)
                }

                bool additional_flag_fall_detect = true;
                switch (mode)
                {
                case SolicareHomeHub::ApiClient::FULL_MONITORING:
                    // TODO: FULL_MONITORING 모드 동작 조건에 따라 이미지 판단
                    break;
                case SolicareHomeHub::ApiClient::CAMERA_ONLY:
                    // TODO: CAMERA_ONLY 모드 동작 조건에 따라 이미지 판단
                    break;
                case SolicareHomeHub::ApiClient::WEARABLE_ONLY:
                    // TODO: WEARABLE_ONLY 모드 동작 조건에 따라 설정 따라 이미지 판단
                    break;
                default:
                    log_warn(TAG, "모든 장치가 분리되었거나 모니터링 상태가 올바르지 않습니다.");
                    mode = wearable_detached ? SolicareHomeHub::ApiClient::CAMERA_ONLY
				                                                                     : SolicareHomeHub::ApiClient::WEARABLE_ONLY;
                }

                // 우선은 둘 중 하나 감지시 바로 푸시
                if (additional_flag_fall_detect && (camera_fallen_detected || wearable_fallen_detected))
                {
                    log_warn(TAG, "[EVENT] 낙상이 감지되었습니다. 보호자에게 알림을 전송합니다");
                    postSeniorAlertEvent(monitorEventToString(SolicareHomeHub::ApiClient::FALL_DETECTED),
				                                                                 monitorModeToString(mode), "");
                }
                // 착용 후 10초가 지나야 센서 스탯을 전송
                if (!wearable_detached && last_wear_gap > TIME_TO_WAIT_DATA)
                {
                    postSeniorStats(camera_fallen_detected, wearable_fallen_detected, temperature_avg, humidity_avg,
				                                                            bpm_avg, battery_avg);
                }
                previous_mode = mode;
                std::this_thread::sleep_for(5s);
            }
        });
}

void SolicareCentralHomeHub::stop_monitoring()
{
	if (!monitoring_active_)
		return;
	monitoring_active_ = false;
	log_info(TAG, "Solicare 시니어 케어 모니터링 서비스를 종료합니다.", LOG_COLOR);
	if (monitoring_thread_.joinable())
		monitoring_thread_.join();
}
