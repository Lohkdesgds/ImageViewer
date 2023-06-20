#include <nlohmann/json.hpp>
#include <Graphics.h>
#include <Console/console.h>

#include "include/updater.h"
#include "include/window.h"
#include "include/configurer.h"

#include <shellapi.h>
#include <chrono>
#include <variant>

using namespace Lunaris;

const uint64_t launch_time_utc = std::chrono::duration_cast<std::chrono::duration<std::uint64_t>>(std::chrono::utc_clock().now().time_since_epoch()).count();
const uint64_t launch_time_delta = 7200; // every 2 hours?

constexpr auto load_bitmap_auto = [](const std::string& p, int mode) -> std::variant<AllegroCPP::Bitmap, AllegroCPP::GIF, bool> {try { AllegroCPP::Bitmap tst(p, mode); if (tst.valid()) return tst; } catch (...) { try { AllegroCPP::GIF tg(p, mode); if (tg.valid()) return tg; } catch (...) { return false; } } return false; };

int main(int argc, char* argv[]) // working dir before: $(ProjectDir)
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
	std::unique_ptr<AllegroCPP::File_tmp> tmp_file_fly;
	AllegroCPP::Event_queue ev_qu;
	bool m_down = false, ctl_down = false;
	struct _async {
		std::vector<std::string> items_in_folder;
		size_t index = 0;
		std::atomic_bool ready = false;
		AllegroCPP::Thread self;
	} dat;
	

	const auto handle_variant = [&window](std::variant<AllegroCPP::Bitmap, AllegroCPP::GIF, bool>&& bmp) { if (std::holds_alternative<AllegroCPP::Bitmap>(bmp)) { window.put(std::move(std::get<AllegroCPP::Bitmap>(bmp))); return true; } else if (std::holds_alternative<AllegroCPP::GIF>(bmp)) { window.put(std::move(std::get<AllegroCPP::GIF>(bmp))); return true; } return false;  };
	const auto put_in_window_from_path_async = [&](const std::string& p) -> std::variant<AllegroCPP::Bitmap, AllegroCPP::GIF, bool> { try { std::variant<AllegroCPP::Bitmap, AllegroCPP::GIF, bool> _res; window.post_wait([p, &_res] { _res = load_bitmap_auto(p, ALLEGRO_MIN_LINEAR | ALLEGRO_VIDEO_BITMAP); }); return _res; } catch (...) { return false; } };

	const auto _async_load_path = [&tmp_file_fly, &dat, &window, &put_in_window_from_path_async, &handle_variant] (const std::string& first_arg) {
		dat.ready = false;
		dat.index = 0;

		auto& gconf = g_conf();
		std::string find_idx = first_arg;

		const auto c1 = gconf.get("history", "last_path");

		if (find_idx.empty() && c1.length()) find_idx = c1;

		if (find_idx.find("http") == 0) { // assume URL

			window.set_text("Downloading from url...");
			window.set_title(" - Downloading...");

			auto mov = std::move(tmp_file_fly); // backup
			tmp_file_fly = std::make_unique<AllegroCPP::File_tmp>();

			const auto buf = http_get(find_idx);

			if (buf.empty()) {
				window.set_text("Could not load from URL.");
				window.set_title("");
				tmp_file_fly = std::move(mov);
				dat.ready = true;
				return false;
			}

			tmp_file_fly->write(buf.data(), buf.size());
			tmp_file_fly->flush();

			if (handle_variant(std::move(put_in_window_from_path_async(tmp_file_fly->get_filepath())))) {
				window.set_text("Loaded " + find_idx);
				window.set_title(" - " + find_idx);
				gconf.set("history", "last_path", find_idx);
				g_save_conf();
			}
			else {
				window.set_text("Could not load from URL.");
				window.set_title("");
				tmp_file_fly = std::move(mov);
			}

			dat.ready = true;
			return false;
		}

		window.set_text("Loading folder in the background...");
		window.set_title(" - Loading...");

		bool has_put_once = false;
		size_t issue_count = 0;

		if (find_idx.size()) {
			has_put_once = handle_variant(std::move(put_in_window_from_path_async(find_idx)));
			if (has_put_once) {
				window.set_title(" - " + std::string(find_idx));
			}
		}

		std::string path = ".";
		for (auto& i : find_idx) if (i == '\\') i = '/';

		gconf.set("history", "last_path", find_idx);
		g_save_conf();

		if (std::filesystem::is_directory(find_idx)) {
			path = find_idx;
			find_idx.clear(); // CLEAR IN find_idx BECAUSE IT WON'T SEARCH FOR FOLDER IN ITERATION
		}
		else if (const size_t p = find_idx.rfind('/'); p != std::string::npos) {
			const auto test = find_idx.substr(0, p);
			if (std::filesystem::is_directory(test)) path = test;
		}

		for (const auto& p : std::filesystem::directory_iterator{ path }) {
			try {
				if (!p.is_regular_file()) continue;
				std::string pstr = std::filesystem::canonical(p.path()).string();
				for (auto& i : pstr) if (i == '\\') i = '/';


				if (!find_idx.empty() && pstr == find_idx) {
					dat.index = dat.items_in_folder.size();
				}

				if (!has_put_once) {
					has_put_once = handle_variant(std::move(put_in_window_from_path_async(pstr)));
					if (has_put_once) {
						window.set_title(" - " + pstr);
					}
					dat.index = dat.items_in_folder.size();
				}

				dat.items_in_folder.push_back(pstr);
			}
			catch (...) {
				++issue_count;
			}
		}

		window.set_text("Found possibly " + std::to_string(dat.items_in_folder.size()) + " images in this folder." + (issue_count > 0 ? (" Issues: " + std::to_string(issue_count)) : ""));
		//al_rest(5.0);
		//window.set_text("");
		dat.ready = true;
		return false;
	};

	dat.self.create([&] { return _async_load_path(argc > 1 ? argv[1] : ""); }, AllegroCPP::Thread::Mode::NORMAL);

	while (!window.has_display()) std::this_thread::sleep_for(std::chrono::milliseconds(25));

	ev_qu << window;
	ev_qu << AllegroCPP::Event_mouse();
	ev_qu << AllegroCPP::Event_keyboard();

	//{
	//	AllegroCPP::Bitmap bmp(argc > 1 ? argv[1] : "test.jpg", ALLEGRO_MEMORY_BITMAP);
	//	window.put(std::move(bmp), true);
	//}

	const auto func_adv_back = [&](int dir) {
		if (dat.items_in_folder.size() <= 1) return;
		if (dir >= 0) {
			window.set_text("Loading image, please wait...");
			window.set_title(" - Loading...");
			for (size_t p = 0; p < dat.items_in_folder.size(); ++p) {
				if (++dat.index >= dat.items_in_folder.size()) dat.index = 0;

				window.set_text("Loading " + std::to_string(dat.index + 1) + "/" + std::to_string(dat.items_in_folder.size()) + "...", 1e20);

				if (handle_variant(std::move(put_in_window_from_path_async(dat.items_in_folder[dat.index])))) {
					window.set_text("Loaded " + std::to_string(dat.index + 1) + "/" + std::to_string(dat.items_in_folder.size()) + ".");
					window.set_title(" - " + dat.items_in_folder[dat.index]);
					gconf.set("history", "last_path", dat.items_in_folder[dat.index]);
					break;
				}

			}
		}
		else {
			window.set_text("Loading image, please wait...");
			window.set_title(" - Loading...");
			for (size_t p = 0; p < dat.items_in_folder.size(); ++p) {
				if (dat.index-- == 0) dat.index = dat.items_in_folder.size() - 1;

				window.set_text("Loading " + std::to_string(dat.index + 1) + "/" + std::to_string(dat.items_in_folder.size()) + "...", 1e20);

				if (handle_variant(std::move(put_in_window_from_path_async(dat.items_in_folder[dat.index])))) {
					window.set_text("Loaded " + std::to_string(dat.index + 1) + "/" + std::to_string(dat.items_in_folder.size()) + ".");
					window.set_title(" - " + dat.items_in_folder[dat.index]);
					gconf.set("history", "last_path", dat.items_in_folder[dat.index]);
					break;
				}
			}
		}

		g_save_conf();
	};
	


	for (bool run = true; run;)
	{
		const auto ev = ev_qu.wait_for_event();

		switch (ev.get().type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			window.set_text("Closing in a second.");
			al_rest(0.5);
			cout << console::color::YELLOW << "Close";
			run = false;
			continue;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			window.ack_resize();
			break;
		case ALLEGRO_EVENT_MOUSE_AXES:
		{
			const auto& mse = ev.get().mouse;

			if (m_down || ctl_down) {
				window.cam().translate(mse.dx, mse.dy);
				window.cam().rotate(mse.dz);
				window.post_update();
			}
			else if (mse.dz != 0) {
				window.cam().scale(mse.dz);
				window.post_update();
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
			ctl_down |= (ev.get().keyboard.keycode == ALLEGRO_KEY_LCTRL || ev.get().keyboard.keycode == ALLEGRO_KEY_RCTRL);

			switch (ev.get().keyboard.keycode) {
			case ALLEGRO_KEY_R:
				window.cam().reset();
				break;
			case ALLEGRO_KEY_RIGHT:
				if (!dat.ready) break;
				dat.self.stop();
				dat.self.create([&] {func_adv_back(1); return false; }, AllegroCPP::Thread::Mode::NORMAL);
				break;
			case ALLEGRO_KEY_LEFT:
				if (!dat.ready) break;
				dat.self.stop();
				dat.self.create([&] {func_adv_back(-1); return false; }, AllegroCPP::Thread::Mode::NORMAL);
				break;
			case ALLEGRO_KEY_F11:
				window.toggle_fullscreen();
				break;
			case ALLEGRO_KEY_V:
				if (ctl_down) {
					if (!dat.ready) break;
					dat.self.stop();
					if (auto s = window.clipboard_text(); s.size()) {
						dat.self.create([&dat, &_async_load_path, s] {
							dat.items_in_folder.clear();
							return _async_load_path(s);
						}, AllegroCPP::Thread::Mode::NORMAL);
					}
				}
				break;
			}
			break;
		case ALLEGRO_EVENT_KEY_UP:
			if (ev.get().keyboard.keycode == ALLEGRO_KEY_LCTRL || ev.get().keyboard.keycode == ALLEGRO_KEY_RCTRL) ctl_down = false;
		}
	}

	return 0;
}