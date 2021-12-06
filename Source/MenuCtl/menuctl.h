#pragma once

#include <Lunaris/all.h>
#include "../Window/window.h"

using namespace Lunaris;

enum class e_menu_item : uint16_t {
	__START = (1 << 12) - 1,

	FILE,
		FILE_OPEN,
		FILE_OPEN_FOLDER,
		FILE_OPEN_URL_CLIPBOARD,
		FILE_EXIT_APP,
	LAYERS_ROOT,
		// layers here
	WINDOW,
		WINDOW_FRAMELESS,
		WINDOW_NOBAR,
		WINDOW_FULLSCREEN,

	RC_WINDOW,
		RC_WINDOW_FRAMELESS,
		RC_WINDOW_NOBAR,
		RC_WINDOW_FULLSCREEN,
	RC_LAYERS_ROOT,
		// layers here
	RC_ROTATE,
		RC_ROTATE_RIGHT,
		RC_ROTATE_LEFT,
	RC_EXIT

};

constexpr uint16_t operator+(const e_menu_item& i) { return static_cast<uint16_t>(i); };

std::vector<menu_quick> gen_menu_bar();
std::vector<menu_quick> gen_menu_pop();

class MenuCtl : public NonCopyable, public NonMovable {
	display& disp;
	bool is_bar_enabled = false;
	menu mn_bar;
	menu mn_pop;
	generic_event_handler mn_evhlr;

	void on_menu(menu_event&);
public:
	MenuCtl(display&);

	void update_frames(const safe_vector<Frame>&);

	void show_pop();
	void show_bar();
	void hide_bar();
};