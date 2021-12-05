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
	safe_vector<Frame> frames;

	void on_disp(const display_event&);
	void on_mouse(const int, const mouse::mouse_event&);
public:
	Window();

	bool draw();

	future<bool> push(const std::string&);
	void safe(std::function<void(std::vector<Frame>&)>);

};