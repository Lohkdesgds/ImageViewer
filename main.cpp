#include <nlohmann/json.hpp>
#include <Graphics.h>
#include <Console/console.h>

#include "include/updater.h"
#include "include/window.h"
#include "include/configurer.h"

#include <shellapi.h>
#include <chrono>

using namespace Lunaris;

const uint64_t launch_time_utc = std::chrono::duration_cast<std::chrono::duration<std::uint64_t>>(std::chrono::utc_clock().now().time_since_epoch()).count();
const uint64_t launch_time_delta = 7200; // every 2 hours?

int main(int argc, char* argv[])
{
	auto& gconf = g_conf();

	if (auto c = gconf.get("general", "last_update_time_check"); !c.empty()) {
		const uint64_t last = std::strtoull(c.data(), nullptr, 10);
		gconf.set("general", "last_update_time_check", std::to_string(launch_time_utc));

		if (launch_time_utc > (last + launch_time_delta)) {
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
	}
	else {
		gconf.set("general", "last_update_time_check", std::to_string(launch_time_utc));
	}

	g_save_conf();
	
	

	

	Window window;
	AllegroCPP::Event_queue ev_qu;
	bool m_down = false, ctl_down = false;
	struct _async {
		std::vector<std::string> items_in_folder;
		size_t index = 0;
		std::atomic_bool ready = false;
		AllegroCPP::Thread self;
	} dat;
	

	dat.self.create([&dat, &argc, &argv] {
		const std::string find_idx = argc > 1 ? argv[1] : "";

		for (const auto& p : std::filesystem::directory_iterator{"."})
		{
			if (!p.is_regular_file()) continue;

			std::string pstr = std::filesystem::canonical(p.path()).string();
			for (auto& i : pstr) if (i == '\\') i = '/';

			AllegroCPP::Bitmap tst;

			try {
				tst = AllegroCPP::Bitmap(pstr, ALLEGRO_MEMORY_BITMAP);

				if (tst.valid()) {
					cout << console::color::LIGHT_PURPLE << "+ " << pstr;
					if (!find_idx.empty() && pstr == find_idx) dat.index = dat.items_in_folder.size();
					dat.items_in_folder.push_back(pstr);
				}
			}
			catch (const std::exception& e) {
				cout << console::color::YELLOW << "WARN '" << e.what() << "' > " << pstr;
			}
			catch (...) {
				cout << console::color::YELLOW << "WARN > " << pstr;
			}

			if (!tst.valid()) {
				try {
					AllegroCPP::GIF tstgif(pstr);
					
					if (tstgif.valid()) {
						cout << console::color::GREEN << "+ " << pstr;
						if (!find_idx.empty() && pstr == find_idx) dat.index = dat.items_in_folder.size();
						dat.items_in_folder.push_back(pstr);
					}
					else {
						cout << console::color::RED << "- " << pstr;
					}
				}
				catch (const std::exception& e) {
					cout << console::color::YELLOW << "WARN '" << e.what() << "' - " << pstr;
				}
				catch (...) {
					cout << console::color::YELLOW << "WARN - " << pstr;
				}
			}

		}


		return false;
	}, AllegroCPP::Thread::Mode::NORMAL);

	while (!window.has_display()) std::this_thread::sleep_for(std::chrono::milliseconds(25));

	ev_qu << window;
	ev_qu << AllegroCPP::Event_mouse();
	ev_qu << AllegroCPP::Event_keyboard();

	{
		AllegroCPP::Bitmap bmp(argc > 1 ? argv[1] : "test.jpg", ALLEGRO_MEMORY_BITMAP);
		window.put(std::move(bmp), true);
	}


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