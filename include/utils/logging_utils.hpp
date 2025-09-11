#pragma once
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>

namespace Logger
{
inline std::mutex log_mutex;
inline std::string log_tag_filter;

inline void set_tag_filter(const std::string_view tag)
{
	log_tag_filter = tag;
}

inline void remove_tag_filter()
{
	log_tag_filter.clear();
}

inline std::string current_timestamp()
{
	const auto now       = std::chrono::system_clock::now();
	const auto in_time_t = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

enum class ConsoleColor
{
	RED,
	ORANGE,
	YELLOW,
	GREEN,
	LIME,
	TEAL,
	CYAN,
	BLUE,
	INDIGO,
	PURPLE,
	VIOLET,
	PINK,
	MAGENTA,
	BROWN,
	WHITE,
	GRAY
};

inline std::string colored_text(const ConsoleColor c, const std::string& text)
{
	switch (c)
	{
	case ConsoleColor::RED:
		return "\033[38;5;196m" + text + "\033[0m";
	case ConsoleColor::ORANGE:
		return "\033[38;5;214m" + text + "\033[0m";
	case ConsoleColor::YELLOW:
		return "\033[38;5;226m" + text + "\033[0m";
	case ConsoleColor::GREEN:
		return "\033[38;5;82m" + text + "\033[0m";
	case ConsoleColor::LIME:
		return "\033[38;5;154m" + text + "\033[0m";
	case ConsoleColor::TEAL:
		return "\033[38;5;80m" + text + "\033[0m";
	case ConsoleColor::CYAN:
		return "\033[38;5;51m" + text + "\033[0m";
	case ConsoleColor::BLUE:
		return "\033[38;5;39m" + text + "\033[0m";
	case ConsoleColor::INDIGO:
		return "\033[38;5;61m" + text + "\033[0m";
	case ConsoleColor::PURPLE:
		return "\033[38;5;135m" + text + "\033[0m";
	case ConsoleColor::VIOLET:
		return "\033[38;5;183m" + text + "\033[0m";
	case ConsoleColor::PINK:
		return "\033[38;5;212m" + text + "\033[0m";
	case ConsoleColor::MAGENTA:
		return "\033[38;5;201m" + text + "\033[0m";
	case ConsoleColor::BROWN:
		return "\033[38;5;180m" + text + "\033[0m";
	case ConsoleColor::WHITE:
		return "\033[38;5;15m" + text + "\033[0m";
	case ConsoleColor::GRAY:
	default:
		return "\033[38;5;250m" + text + "\033[0m";
	}
}

inline std::string colored_text_256(const int color_code, const std::string& text)
{
	return "\033[38;5;" + std::to_string(color_code) + "m" + text + "\033[0m";
}

inline std::string colored_text_rgb(const int r, const int g, const int b, const std::string& text)
{
	return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m" + text +
	       "\033[0m";
}

inline void log_info(const std::string_view tag, const std::string& message,
                     const ConsoleColor color = ConsoleColor::GRAY)
{
	if (!log_tag_filter.empty() && tag != log_tag_filter)
		return;
	std::lock_guard lock(log_mutex);
	std::cout << colored_text(color, "[" + current_timestamp() + "] [" + std::string(tag) + "] " + message)
	          << std::endl;
}

inline void log_warn(const std::string_view tag, const std::string& message)
{
	if (!log_tag_filter.empty() && tag != log_tag_filter)
		return;
	std::lock_guard lock(log_mutex);
	std::cout << colored_text(ConsoleColor::ORANGE,
	                          "[" + current_timestamp() + "] [" + std::string(tag) + "] " + message)
	          << std::endl;
}

inline void log_error(const std::string_view tag, const std::string& message)
{
	if (!log_tag_filter.empty() && tag != log_tag_filter)
		return;
	std::lock_guard lock(log_mutex);
	std::cerr << colored_text(ConsoleColor::RED, "[" + current_timestamp() + "] [" + std::string(tag) + "] " + message)
	          << std::endl;
}

inline void preview_all_colors()
{
	std::cout << "\n--- ConsoleColor Preview (Rainbow Order) ---\n";
	std::cout << colored_text(ConsoleColor::RED, "테스트 문구 입니다 (RED: 빨강)") << std::endl;
	std::cout << colored_text(ConsoleColor::ORANGE, "테스트 문구 입니다 (ORANGE: 오렌지)") << std::endl;
	std::cout << colored_text(ConsoleColor::YELLOW, "테스트 문구 입니다 (YELLOW: 노랑)") << std::endl;
	std::cout << colored_text(ConsoleColor::GREEN, "테스트 문구 입니다 (GREEN: 초록)") << std::endl;
	std::cout << colored_text(ConsoleColor::LIME, "테스트 문구 입니다 (LIME: 라임)") << std::endl;
	std::cout << colored_text(ConsoleColor::TEAL, "테스트 문구 입니다 (TEAL: 청록)") << std::endl;
	std::cout << colored_text(ConsoleColor::CYAN, "테스트 문구 입니다 (CYAN: 시안)") << std::endl;
	std::cout << colored_text(ConsoleColor::BLUE, "테스트 문구 입니다 (BLUE: 파랑)") << std::endl;
	std::cout << colored_text(ConsoleColor::INDIGO, "테스트 문구 입니다 (INDIGO: 인디고)") << std::endl;
	std::cout << colored_text(ConsoleColor::PURPLE, "테스트 문구 입니다 (PURPLE: 보라)") << std::endl;
	std::cout << colored_text(ConsoleColor::VIOLET, "테스트 문구 입니다 (VIOLET: 바이올렛)") << std::endl;
	std::cout << colored_text(ConsoleColor::PINK, "테스트 문구 입니다 (PINK: 핑크)") << std::endl;
	std::cout << colored_text(ConsoleColor::MAGENTA, "테스트 문구 입니다 (MAGENTA: 마젠타)") << std::endl;
	std::cout << colored_text(ConsoleColor::BROWN, "테스트 문구 입니다 (BROWN: 갈색)") << std::endl;
	std::cout << colored_text(ConsoleColor::WHITE, "테스트 문구 입니다 (WHITE: 흰색)") << std::endl;
	std::cout << colored_text(ConsoleColor::GRAY, "테스트 문구 입니다 (GRAY: 기본값, 회색)") << std::endl;
	std::cout << "---------------------------\n" << std::endl;
}
} // namespace Logger