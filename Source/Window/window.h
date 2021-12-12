#pragma once

#include <Lunaris/all.h>
#include <regex>
#include "../shared.h"
#include "../Frame/frame.h"
#include "../../resource.h"

using namespace Lunaris;


class Window : public NonCopyable, public NonMovable {
	display disp;
	display_event_handler dispev;
	mouse mous;
	safe_vector<Frame> frames; // 0 is on top of the others (and in focus)
	safe_data<std::function<void(const safe_vector<Frame>&)>> f_frames_update;
	safe_data<std::function<void(const size_t, Frame&)>> f_rclick;

	size_t last_right_click_n = static_cast<size_t>(-1);

	void on_disp(const display_event&);
	void on_mouse(const int, const mouse::mouse_event&);
public:
	Window();

	bool draw();

	future<bool> push(const std::string&);
	void safe(std::function<void(std::vector<Frame>&)>);

	void on_frames_update(std::function<void(const safe_vector<Frame>&)>);
	void on_right_click(std::function<void(const size_t, Frame&)>);
	void post_right_click(std::function<void(Frame&)>);

	const display& get_display() const;
	display& get_display();

	bool put_on_top(const size_t);
};