#include "solicare_central_home_hub.hpp"

using namespace std;

using namespace Logger;

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
	std::cout << "  1. 웹소켓 서버 실행  | Start WebSocket Server" << std::endl;
	std::cout << "     - 비동기 웹소켓 서버를 실행합니다." << std::endl;
	std::cout << "  2. 웹소켓 서버 중지  | Stop WebSocket Server" << std::endl;
	std::cout << "     - 실행 중인 웹소켓 서버를 중지합니다." << std::endl;
	std::cout << "  3. 프로그램 종료     | Exit Program" << std::endl;
	std::cout << "     - Solicare Central Home Hub을 종료합니다." << std::endl;
	std::cout << string(width, '-') << std::endl;

	int choice = 0;
	while (true)
	{
		std::cout << "* 메뉴 번호를 입력하세요 (1~3): ";
		cin >> choice;
		if (cin.fail())
		{
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			std::cout << "잘못된 입력입니다. 숫자를 입력하세요.\n" << std::endl;
			continue;
		}
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		if (choice < 1 || choice > 3)
		{
			std::cout << colored_text(ConsoleColor::RED, "잘못된 선택입니다.\n") << std::endl;
			continue;
		}
		return choice;
	}
}
