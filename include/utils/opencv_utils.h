#pragma once

#include <functional>
#include <opencv2/opencv.hpp>
#include <string>

#define CV_FONT_SCALE 0.8
#define CV_FONT_THICKNESS 2
#define CV_TEXT_MARGIN 10

enum TEXT_LOCATION
{
	TEXT_TOP_LEFT,
	TEXT_TOP_RIGHT,
	TEXT_BOTTOM_LEFT,
	TEXT_BOTTOM_RIGHT
};

enum TEXT_COLOR
{
	COLOR_WHITE,
	COLOR_BLACK,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_BLUE,
	COLOR_YELLOW
};

inline cv::Scalar getColor(const TEXT_COLOR font_color)
{
	switch (font_color)
	{
	case COLOR_WHITE:
		return {255, 255, 255};
	case COLOR_BLACK:
		return {0, 0, 0};
	case COLOR_RED:
		return {0, 0, 255};
	default:
	case COLOR_GREEN:
		return {0, 255, 0};
	case COLOR_BLUE:
		return {255, 0, 0};
	case COLOR_YELLOW:
		return {0, 255, 255};
	}
}

inline void put_text_overlay(const cv::Mat& image, const std::string& text, const TEXT_LOCATION location,
                             const TEXT_COLOR text_color)
{
	const auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, CV_FONT_SCALE, CV_FONT_THICKNESS, nullptr);

	const std::pair<TEXT_LOCATION, std::function<cv::Point()>> loc_map[] = {
	    {TEXT_TOP_LEFT, [=] { return cv::Point{CV_TEXT_MARGIN, 15 + CV_TEXT_MARGIN}; }},
	    {TEXT_TOP_RIGHT,
	     [=] { return cv::Point{image.cols - (text_size.width + CV_TEXT_MARGIN), 15 + CV_TEXT_MARGIN}; }},
	    {TEXT_BOTTOM_LEFT, [=] { return cv::Point{CV_TEXT_MARGIN, image.rows - (text_size.height + CV_TEXT_MARGIN)}; }},
	    {TEXT_BOTTOM_RIGHT, [=]
	     {
		     return cv::Point{image.cols - (text_size.width + CV_TEXT_MARGIN),
		                      image.rows - (text_size.height + CV_TEXT_MARGIN)};
	     }}};

	const auto it = std::ranges::find_if(loc_map, [&](const auto& pair) { return pair.first == location; });

	const cv::Point target = (it != std::end(loc_map)) ? it->second() : cv::Point{CV_TEXT_MARGIN, CV_TEXT_MARGIN};

	cv::putText(image, text, target, cv::FONT_HERSHEY_SIMPLEX, CV_FONT_SCALE, getColor(text_color), CV_FONT_THICKNESS);
}

inline void put_text_with_background(cv::Mat& image, const std::string& text, const cv::Point& position,
                                     const TEXT_COLOR text_color = COLOR_WHITE, const TEXT_COLOR bg_color = COLOR_BLACK,
                                     const double font_scale = CV_FONT_SCALE)
{
	const auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, CV_FONT_THICKNESS, nullptr);

	// Draw background rectangle
	const cv::Point bg_top_left(position.x - 2, position.y - text_size.height - 2);
	const cv::Point bg_bottom_right(position.x + text_size.width + 2, position.y + 2);
	cv::rectangle(image, bg_top_left, bg_bottom_right, getColor(bg_color), -1);

	// Draw text
	cv::putText(image, text, position, cv::FONT_HERSHEY_SIMPLEX, font_scale, getColor(text_color), CV_FONT_THICKNESS);
}

inline void draw_colored_circle(cv::Mat& image, const cv::Point& center, const int radius, const TEXT_COLOR color,
                                const int thickness = -1)
{
	cv::circle(image, center, radius, getColor(color), thickness);
}

inline void draw_colored_line(cv::Mat& image, const cv::Point& pt1, const cv::Point& pt2, const TEXT_COLOR color,
                              const int thickness = 2)
{
	cv::line(image, pt1, pt2, getColor(color), thickness);
}
