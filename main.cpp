#include <nlohmann/json.hpp>
#include <Graphics.h>
#include <Console/console.h>

#include "include/updater.h"
#include "include/window.h"

#include <shellapi.h>

using namespace Lunaris;

int main(int argc, char* argv[])
{
	{
		cout << "Checking for updates...";
		Updater update;
		if (update.fetch()) {
			cout << console::color::GREEN << "Has update!";

			const auto res = AllegroCPP::message_box("There is an update! Would you like to download it?", "Update available! " + update.get().remote_title, update.get().remote_body, "", ALLEGRO_MESSAGEBOX_YES_NO);

			if (res == 1) { // "update"
				ShellExecuteA(0, 0, update.get().remote_link.c_str(), 0, 0, SW_SHOW);
				return 0;
			}
		}
		else {
			cout << "Up to update.";
		}
	}

	Window window;
	AllegroCPP::Event_queue ev_qu;
	bool m_down = false, ctl_down = false;

	while (!window.has_display()) std::this_thread::sleep_for(std::chrono::milliseconds(25));

	ev_qu << window;
	ev_qu << AllegroCPP::Event_mouse();
	ev_qu << AllegroCPP::Event_keyboard();


	{
		AllegroCPP::Bitmap bmp(argc > 1 ? argv[1] : "test.jpg", ALLEGRO_MEMORY_BITMAP);
		window.put(std::move(bmp), true);
	}

	std::cout << "Hello world" << std::endl;

	for (bool run = true; run;)
	{
		const auto ev = ev_qu.wait_for_event();

		switch (ev.get().type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			cout << console::color::YELLOW << "Close";
			run = false;
			continue;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			window.ack_resize();
			break;
		case ALLEGRO_EVENT_MOUSE_AXES:
		{
			const auto& mse = ev.get().mouse;

			if (m_down) {
				window.cam().translate(mse.dx, mse.dy);
				window.cam().rotate(mse.dz);
			}
			else {
				window.cam().scale(mse.dz);
			}
		}
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			m_down |= (ev.get().mouse.button == 1);
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			if (ev.get().mouse.button == 1) m_down = false;
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
			ctl_down |= (ev.get().keyboard.keycode == ALLEGRO_KEYMOD_CTRL);
			if (ev.get().keyboard.keycode == ALLEGRO_KEY_R) window.cam().reset();
			break;
		case ALLEGRO_EVENT_KEY_UP:
			if (ev.get().keyboard.keycode == ALLEGRO_KEYMOD_CTRL) ctl_down = false;
		}
	}

	return 0;
}