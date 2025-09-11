#pragma once

namespace InputUtils
{
#if defined(_WIN32)
#include <conio.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

// 플랫폼별 키 입력 감지 함수
// 엔터 없이 키 입력을 즉시 감지 (키가 눌렸으면 해당 문자, 아니면 -1 반환)
inline char get_key_press()
{
#if defined(_WIN32)
	if (_kbhit())
		return _getch();
	return -1;
#else
	// TODO: Non-blocking read implementation needed for non-Windows systems
	termios oldt{}, newt{};
	char ch = 0;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	newt.c_cc[VMIN]  = 1;
	newt.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	const int bytes = read(STDIN_FILENO, &ch, 1);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	if (bytes > 0)
		return ch;
	return -1;
#endif
}
} // namespace InputUtils