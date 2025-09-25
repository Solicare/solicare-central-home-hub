#include "solicare_central_home_hub.hpp"
#include "utils/system_utils.hpp"

using namespace std;
using namespace std::chrono;

using namespace Logger;

using namespace SolicareHomeHub::Monitor;

void SolicareCentralHomeHub::on_menu_monitor_mode()
{
	log_info(TAG, "보호자 제어 모드를 활성화 합니다. 종료하시려면 q키를 누르세요.\n", LOG_COLOR);

	std::atomic monitoring_active{true};
	std::thread monitor_thread(
	    [this, &monitoring_active]()
	    {
		    bool prev_state = false;
		    while (monitoring_active)
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
				    }
				    if (monitoring == false && websocket_server_)
				    {
					    log_info(TAG, "보호자 모니터링 중단 → 실행중인 비동기 웹소켓 서버를 중지합니다.", LOG_COLOR);
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
			    websocket_server_->stop();
			    this_thread::sleep_for(milliseconds(1000));
			    websocket_server_.reset();
		    }
	    });
	while (true)
	{
		if (const char key = InputUtils::get_key_press(); key == 'q' || key == 'Q')
		{
			monitoring_active = false;
			break;
		}
	}
	if (monitor_thread.joinable())
		monitor_thread.join();
}
