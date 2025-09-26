#include "solicare_central_home_hub.hpp"
#include <fmt/core.h>

using namespace std;

using namespace Logger;

void SolicareCentralHomeHub::login()
{
	std::string id, pw;
	bool logged_in = false;
	while (!logged_in)
	{
		constexpr int width = 40;
		std::cout << string(width, '=') << std::endl;
		std::cout << "|" << std::setw(width - 2) << std::setfill(' ') << std::left << "      Solicare Central Home Hub"
		          << "|" << std::endl;
		std::cout << "|" << std::setw(width - 2) << std::setfill('-') << "" << "|" << std::endl;
		std::cout << "|" << std::setw(width - 2) << std::setfill(' ') << "            Please Login" << "|" << std::endl;
		std::cout << "|" << std::setw(width - 2) << std::setfill('-') << "" << "|" << std::endl;
		std::cout << "| Username: 시니어 계정 사용자 ID" << std::setw(width - 30) << std::setfill(' ') << "" << "|"
		          << std::endl;
		std::cout << "| Password: 시니어 계정 사용자 PW" << std::setw(width - 30) << std::setfill(' ') << "" << "|"
		          << std::endl;
		std::cout << string(width, '=') << std::endl;

		std::cout << "Username: ";
		std::getline(std::cin, id);
		std::cout << "Password: ";
		std::getline(std::cin, pw);
		std::cout << std::endl;

		logged_in = process_senior_login(id, pw);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (!logged_in)
		{
			std::cout << colored_text(ConsoleColor::YELLOW, "\nLogin failed. Please try again.") << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		}
	}
	std::cout << colored_text(ConsoleColor::GREEN, fmt::format("\nLogin successful! Welcome, {}!", identity_.name))
	          << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

int SolicareCentralHomeHub::prompt_menu_selection()
{
	if (log_tag_filter.empty())
	{
		set_tag_filter(SolicareHomeHub::Launcher::TAG);
		log_info(SolicareHomeHub::Launcher::TAG, "로그 출력이 제한됩니다.", SolicareHomeHub::Launcher::LOG_COLOR);
	}

	constexpr int width = 40;
	std::cout << string(width, '=') << std::endl;
	std::cout << "      Solicare Central Home Hub" << std::endl;
	std::cout << string(width, '-') << std::endl;
	std::cout << "  1. 보호자 제어 모니터링 시작  | Start Monitoring Service" << std::endl;
	std::cout << "     - 보호자에 의해 제어되는 모니터링 서비스를 시작합니다." << std::endl;
	std::cout << "  2. 웹소켓 서버 실행  | Start WebSocket Server" << std::endl;
	std::cout << "     - 수동으로 비동기 웹소켓 서버를 실행합니다." << std::endl;
	std::cout << "  3. 웹소켓 서버 중지  | Stop WebSocket Server" << std::endl;
	std::cout << "     - 수동으로 실행 중인 웹소켓 서버를 중지합니다." << std::endl;
	std::cout << "  4. 프로그램 종료     | Exit Program" << std::endl;
	std::cout << "     - Solicare Central Home Hub을 종료합니다." << std::endl;
	std::cout << string(width, '-') << std::endl;

	int choice = 0;
	while (true)
	{
		std::cout << "* 메뉴 번호를 입력하세요 (1~4): ";
		cin >> choice;
		if (cin.fail())
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			std::cout << "잘못된 입력입니다. 숫자를 입력하세요.\n" << std::endl;
			continue;
		}
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		if (choice < 1 || choice > 4)
		{
			std::cout << colored_text(ConsoleColor::RED, "잘못된 선택입니다.\n") << std::endl;
			continue;
		}
		return choice;
	}
}