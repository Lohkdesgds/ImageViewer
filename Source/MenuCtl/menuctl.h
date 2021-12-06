#pragma once

#include <Lunaris/all.h>
#include "../Window/window.h"

using namespace Lunaris;

enum class e_menu_item : uint16_t {
	__START = (1 << 12) - 1,

	FILE,
		LAYERS_ROOT,

	RC_ROTATE,
		RC_ROTATE_RIGHT,
		RC_ROTATE_LEFT
};

constexpr uint16_t operator+(const e_menu_item& i) { return static_cast<uint16_t>(i); };

std::vector<menu_quick> gen_menu_bar();
std::vector<menu_quick> gen_menu_pop();

class MenuCtl : public NonCopyable, public NonMovable {
	menu mn_bar;
	menu mn_pop;
public:
	MenuCtl();

	void update_frames(const safe_vector<Frame>&);

	void show_pop(const display&);
	void show_bar(const display&);
	void hide_bar(const display&);
};