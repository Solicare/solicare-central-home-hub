#include "solicare_central_home_hub.hpp"
#include "utils/opencv_utils.hpp"
#include "utils/system_utils.hpp"

using namespace std;
using namespace chrono;

using namespace Logger;
using namespace SolicareHomeHub::Launcher;

SolicareCentralHomeHub::SolicareCentralHomeHub()
{
	using SolicareHomeHub::CameraProcessor::pose_net;
	const int cuda_devices_count = OpenCVUtils::enable_cuda_devices();
	pose_net.emplace(cv::dnn::readNetFromONNX(SolicareHomeHub::CameraProcessor::YOLOV8_POSE_MODEL_PATH));
	if (cuda_devices_count > 0)
	{
		try
		{
			pose_net->setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
			pose_net->setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
			log_info(TAG, "Sucessfully set CUDA backend and target to DNN model.", ConsoleColor::GREEN);
		}
		catch (const std::exception& e)
		{
			log_warn(TAG, fmt::format("Failed to set CUDA backend and target to DNN model: {}", e.what()));
			pose_net->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
			pose_net->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
		}
	}
	else
	{
		log_warn(TAG, "No CUDA device found. Using CPU backend for DNN model.");
		pose_net->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
		pose_net->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
	}
	ioc_work_guard_.emplace(ioc_.get_executor());
	io_context_run_thread_ = std::thread(
	    [this]()
	    {
		    ioc_.run();
		    try
		    {
			    ioc_.run();
		    }
		    catch (const boost::system::system_error& e)
		    {
			    if (e.code() != boost::asio::error::operation_aborted)
			    {
				    log_error(TAG,
				              fmt::format("io_context.run() system_error: {} (code: {})", e.what(), e.code().value()));
			    }
		    }
		    catch (const std::exception& e)
		    {
			    log_error(TAG, fmt::format("io_context.run() exception: {}", e.what()));
			    ioc_.stop();
		    }
		    catch (...)
		    {
			    log_error(TAG, "io_context.run() unknown exception occurred");
			    ioc_.stop();
		    }
	    });
	log_info(TAG, "Sucessfully initialized Solicare Central Home Hub.", LOG_COLOR);
}

SolicareCentralHomeHub::~SolicareCentralHomeHub()
{
	log_info(TAG, "Solicare Central Home Hub을 종료합니다...", LOG_COLOR);
	if (websocket_server_)
	{
		websocket_server_->stop(); // 서버 안전 종료
		websocket_server_.reset();
	}
	if (ioc_work_guard_)
	{
		ioc_work_guard_->reset();
		ioc_work_guard_.reset();
	}
	ioc_.stop();
	if (io_context_run_thread_.joinable())
	{
		io_context_run_thread_.join();
	}
}

void SolicareCentralHomeHub::runtime()
{
	bool running   = true;
	log_tag_filter = TAG;
	while (running)
	{
		if (websocket_server_)
		{
			if (log_tag_filter == TAG)
			{
				log_info(TAG, "로그 출력 제한이 해제되었습니다.", ConsoleColor::GREEN);
				remove_tag_filter();
			}

			log_info(TAG,
			         fmt::format("서버가 {} 포트에서 실행 중입니다. 로그 출력을 끄고 메뉴를 표시하려면 'm'을 누르세요.",
			                     WebSocketServerContext::ws_server_config.server_port),
			         ConsoleColor::GREEN);

			while (true)
			{
				char key;
				while (key = InputUtils::get_key_press(), key == -1)
				{
					this_thread::sleep_for(milliseconds(50));
				}

				if (key == 'm' || key == 'M')
				{
					break;
				}
			}
		}

		switch (prompt_menu_selection())
		{
		case 1:
		{
			if (log_tag_filter == TAG)
			{
				log_info(TAG, "로그 출력 제한이 해제되었습니다.", ConsoleColor::GREEN);
				remove_tag_filter();
			}
			on_menu_monitor_mode();
			break;
		}
		case 2:
		{
			if (!websocket_server_)
			{
				on_menu_server_start();
			}
			break;
		}
		case 3:
			on_menu_server_stop();
			break;
		case 4:
			running = false;
			on_menu_server_stop();
			break;
		default:
			std::cout << colored_text(ConsoleColor::RED, "잘못된 메뉴 입력입니다.\n") << std::endl;
			break;
		}
	}
}

void SolicareCentralHomeHub::on_menu_server_start()
{
	if (!websocket_server_)
	{
		unsigned short port = 0;
		while (true)
		{
			std::cout << "서버 포트 입력 (1024~65535 권장): ";
			cin >> port;
			if (cin.fail())
			{
				cin.clear();
				cin.ignore(numeric_limits<streamsize>::max(), '\n');
				std::cout << colored_text(ConsoleColor::YELLOW, "잘못된 입력입니다. 숫자를 입력하세요.\n") << std::endl;
				continue;
			}
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			if (port < 1 || port > 65535)
			{
				std::cout << colored_text(ConsoleColor::RED, "잘못된 포트입니다. 1~65535 사이의 값을 입력하세요.\n")
				          << std::endl;
				continue;
			}
			break;
		}
		try
		{
			remove_tag_filter();
			WebSocketServerContext::flag_stop_server             = false;
			WebSocketServerContext::ws_server_config.server_port = port;
			WebSocketServerContext::ws_session_map.clear();
			websocket_server_ = make_shared<AsyncWebSocketServer>(ioc_);
			websocket_server_->start();
		}
		catch (const boost::system::system_error& e)
		{
			log_error(TAG, fmt::format("서버 바인딩 실패: {}", e.what()));
			this_thread::sleep_for(milliseconds(500));
			if (port <= 1024)
			{
				std::cout << colored_text(ConsoleColor::RED,
				                          "1024 이하 포트는 관리자 권한이 필요합니다. "
				                          "1024 이상의 포트를 사용하거나 관리자 권한으로 실행하세요.\n")
				          << std::endl;
			}
			else
			{
				std::cout << colored_text(ConsoleColor::RED,
				                          "포트가 이미 사용 중이거나, 방화벽/보안 정책에 의해 제한될 수 있습니다.\n")
				          << std::endl;
			}
		}
	}
	else
	{
		log_warn(TAG, "서버 실행 거부: 이미 실행 중인 서버가 있습니다.\n");
	}
	this_thread::sleep_for(milliseconds(1000));
}

void SolicareCentralHomeHub::on_menu_server_stop()
{
	remove_tag_filter();
	cv::waitKey(1);
	cv::destroyAllWindows();
	if (websocket_server_)
	{
		log_info(TAG, "서버 종료를 대기하는 중 입니다.", LOG_COLOR);
		websocket_server_->stop();
		this_thread::sleep_for(milliseconds(1000));
		websocket_server_.reset();
		log_info(TAG, "서버가 성공적으로 중지되었습니다.\n", ConsoleColor::GREEN);
	}
	else
	{
		log_warn(TAG, "서버 중지 거부: 실행 중인 서버가 없습니다.\n");
	}
	this_thread::sleep_for(milliseconds(1000));
}

int main()
{
	SolicareCentralHomeHub hub;
	// hub.login();
	hub.runtime();
	return 0;
}