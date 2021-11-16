#include <Lunaris/all.h>

#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_windows.h>

#include <nlohmann/json.hpp>

#include "resource.h"

#include "window_control.h"
#include "configuration.h"
#include "shared.h"

#define IMGVW_STANDARD_START_MAIN Lunaris::console::color::BLUE << "[MAINCL] " << Lunaris::console::color::GRAY 

using namespace Lunaris;

std::vector<std::string> parse_call(const int, const char*[]);
std::vector<std::string> recover_conf_files(const ImageViewer::ConfigManager&);
std::vector<std::string> get_files_in_folder(const std::string&);
hybrid_memory<file> get_URL(const std::string&);
std::string get_final_name(std::string);

int main(const int argc, const char* argv[])
{
	cout << IMGVW_STANDARD_START_MAIN << "Starting Image Viewer...";

	std::atomic<size_t> index = 0;
	std::atomic_bool keep_rolling = true;
	ImageViewer::ConfigManager conf;
	ImageViewer::WindowControl window(conf);
	mouse mousectl(window.get_raw_display());
	keys kbctl;

	safe_vector<std::string> vec;
	safe_vector<display_event> evvs;

	cout << IMGVW_STANDARD_START_MAIN << "Parsing call and config files...";

	vec.safe([&](auto& vc) {vc = parse_call(argc, argv); });
	if (!vec.size()) vec.safe([&](auto& vc) {vc = recover_conf_files(conf); });

	cout << IMGVW_STANDARD_START_MAIN << "Setting up window quit and stuff...";

	window.on_quit([&] {keep_rolling = false; });
	window.on_menu([&](menu_event& ev) {
		switch (ev.get_id()) {
		case static_cast<uint16_t>(ImageViewer::WindowControl::event_menu::FILE_OPEN):
		{
			bomb retro([&] {window.set_mode(ImageViewer::WindowControl::display_mode::SHOW); });

			window.set_mode(ImageViewer::WindowControl::display_mode::LOADING);

			const auto dialog = std::unique_ptr<ALLEGRO_FILECHOOSER, void(*)(ALLEGRO_FILECHOOSER*)>(al_create_native_file_dialog(nullptr, u8"Open a file as image", "*.jpg;*.png;*.bmp;*.jpeg;*.gif;*.*", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST | ALLEGRO_FILECHOOSER_PICTURES), al_destroy_native_file_dialog);
			const bool got_something = al_show_native_file_dialog(window.get_raw_display().get_raw_display(), dialog.get()) && al_get_native_file_dialog_count(dialog.get()) > 0;

			if (!got_something) return;

			std::string path = al_get_native_file_dialog_path(dialog.get(), 0);
			index = 0;

			auto fp = make_hybrid<file>();
			if (fp->open(path, file::open_mode_e::READ_TRY)) {
				cout << IMGVW_STANDARD_START_MAIN << " Loaded from Open: " << path;
				vec.clear();
				vec.push_back(std::string(path));
				conf.write_conf().set("general", "last_seen_list", std::vector<std::string>{ path });

				window.replace_image(std::move(fp), get_final_name(path));
			}
			else {
				std::string __nam = "Could not open file '" + path + "'";
				al_show_native_message_box(nullptr, "Can't open file", "Failed opening file", __nam.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
			}
		}
		break;
		case static_cast<uint16_t>(ImageViewer::WindowControl::event_menu::FILE_FOLDER_OPEN):
		{
			bomb retro([&] {window.set_mode(ImageViewer::WindowControl::display_mode::SHOW); });

			window.set_mode(ImageViewer::WindowControl::display_mode::LOADING);

			const auto dialog = std::unique_ptr<ALLEGRO_FILECHOOSER, void(*)(ALLEGRO_FILECHOOSER*)>(al_create_native_file_dialog(nullptr, u8"Search files in path...", "*.*", ALLEGRO_FILECHOOSER_FOLDER), al_destroy_native_file_dialog);
			const bool got_something = al_show_native_file_dialog(window.get_raw_display().get_raw_display(), dialog.get()) && al_get_native_file_dialog_count(dialog.get()) > 0;

			if (!got_something) return;

			std::string path = al_get_native_file_dialog_path(dialog.get(), 0);

			auto pott = get_files_in_folder(path);

			if (pott.size()) {
				conf.write_conf().set("general", "last_seen_list", pott);
				vec.safe([&](std::vector<std::string>& vv) { vv = std::move(pott); });
				index = 0;
			}
			else {
				al_show_native_message_box(nullptr, "This folder isn't great...", "Empty folder or no match!", "None of the files found in the folder is a compatible image.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
				return;
			}

			if (vec.size()) {
				auto fp = make_hybrid<file>();
				if (fp->open(vec.index(0), file::open_mode_e::READ_TRY))
					window.replace_image(std::move(fp), get_final_name(vec.index(0)));
			}
		}
		break;
		case static_cast<uint16_t>(ImageViewer::WindowControl::event_menu::FILE_PASTE_URL):
		{
			bomb retro([&] {window.set_mode(ImageViewer::WindowControl::display_mode::SHOW); });
			window.set_mode(ImageViewer::WindowControl::display_mode::LOADING);

			const std::string path = window.get_raw_display().clipboard().get_text();
			if (path.empty()) break;

			hybrid_memory<file> fp;

			if (path.find("http") == 0) {
				fp = get_URL(path);

				if (fp.valid() && fp->is_open()) {
					cout << IMGVW_STANDARD_START_MAIN << " Loaded from FILE_PASTE_URL (url): " << path;
					vec.clear();
					vec.push_back(std::string(path));
					conf.write_conf().set("general", "last_seen_list", std::vector<std::string>{ path });
					window.replace_image(std::move(fp), get_final_name(path));
				}
				else {
					std::string __nam = "Could not open URL '" + path + "'";
					al_show_native_message_box(nullptr, "Can't open URL", "Failed opening URL from clipboard", __nam.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
					return;
				}
			}
			else {
				fp = make_hybrid<file>();

				if (fp->open(path, file::open_mode_e::READ_TRY)) {
					cout << IMGVW_STANDARD_START_MAIN << " Loaded from FILE_PASTE_URL: " << path;
					vec.clear();
					vec.push_back(std::string(path));
					conf.write_conf().set("general", "last_seen_list", std::vector<std::string>{ path });
					window.replace_image(std::move(fp), get_final_name(path));
				}
				else {
					auto pott = get_files_in_folder(path);

					if (pott.size()) {
						conf.write_conf().set("general", "last_seen_list", pott);
						vec.safe([&](std::vector<std::string>& vv) { vv = std::move(pott); });
						index = 0;
					}
					else {
						std::string __nam = "Could not open file or folder '" + path + "'";
						al_show_native_message_box(nullptr, "Can't open file/folder", "Failed opening file/folder from clipboard", __nam.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
						return;
					}

					if (vec.size()) {
						auto fp = make_hybrid<file>();
						if (fp->open(vec.index(0), file::open_mode_e::READ_TRY))
							window.replace_image(std::move(fp), get_final_name(vec.index(0)));
					}
				}
			}
		}
		break;
		}
	});

	cout << IMGVW_STANDARD_START_MAIN << "Hooking mouse events...";

	mousectl.hook_event([&](const int id, const mouse::mouse_event& ev) {
		static float mx_c = 0.0f, my_c = 0.0f;// , my_r = 0.0f; // save state for movement
			//ma_y_c = 0.0f;// , old_r = 0.0f;// , ma_x_c = 0.0f;// , old_z = 0.0f; // save state for alt

		switch (id) {
		case ALLEGRO_EVENT_MOUSE_AXES:
			if (ev.is_button_pressed(0)) {
				window.set_camera_x(ev.real_posx + mx_c);
				window.set_camera_y(ev.real_posy + my_c);
			}
			else if (ev.got_scroll_event())
			{
				if (kbctl.is_key_pressed(ALLEGRO_KEY_ALT)) {
					window.set_camera_r((ev.scroll_event_id(1) * 0.01f * ALLEGRO_PI) + window.get_camera_r());
				}
				else {
					const float rn = window.get_camera_z();
					if (rn < 0.25f) {
						const float val = rn + ev.scroll_event_id(1) * 0.0125f;
						if (val > 0.0f) window.set_camera_z(val);
					}
					else if (rn < 0.5f) {
						window.set_camera_z(rn + ev.scroll_event_id(1) * 0.05f);
					}
					else if (rn < 1.5f) {
						window.set_camera_z(rn + ev.scroll_event_id(1) * 0.10f);
					}
					else if (rn < 5.0f) {
						window.set_camera_z(rn + ev.scroll_event_id(1) * 0.25f);
					}
					else if (rn < 25.0f) {
						window.set_camera_z(rn + ev.scroll_event_id(1) * 1.0f);
					}
					else {
						window.set_camera_z(rn + ev.scroll_event_id(1) * 10.0f);
					}
				}
			}
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			if (ev.is_button_pressed(0)) {
				mx_c = window.get_camera_x() - ev.real_posx;
				my_c = window.get_camera_y() - ev.real_posy;
			}
			/*else {
				window.right_click();
			}*/
			break;
		}
	});

	cout << IMGVW_STANDARD_START_MAIN << "Hooking keyboard events...";

	kbctl.hook_event([&](const keys::key_event& ev) {
		// common controls

		if (ev.down) {
			switch (ev.key_id) {
			case ALLEGRO_KEY_V:
			{
				if (!kbctl.is_key_pressed(ALLEGRO_KEY_LCTRL)) break;
				// right now has LCTRL + V

				bomb retro([&] {window.set_mode(ImageViewer::WindowControl::display_mode::SHOW); });
				window.set_mode(ImageViewer::WindowControl::display_mode::LOADING);

				const std::string path = window.get_raw_display().clipboard().get_text();
				if (path.empty()) break;

				hybrid_memory<file> fp;

				if (path.find("http") == 0) {
					fp = get_URL(path);

					if (fp.valid() && fp->is_open()) {
						cout << IMGVW_STANDARD_START_MAIN << " Loaded from CTRL V (url): " << path;
						vec.clear();
						vec.push_back(std::string(path));
						conf.write_conf().set("general", "last_seen_list", std::vector<std::string>{ path });
						window.replace_image(std::move(fp), get_final_name(path));
					}
					else {
						std::string __nam = "Could not open URL '" + path + "'";
						al_show_native_message_box(nullptr, "Can't open URL", "Failed opening URL from clipboard", __nam.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
						return;
					}
				}
				else {
					fp = make_hybrid<file>();

					if (fp->open(path, file::open_mode_e::READ_TRY)) {
						cout << IMGVW_STANDARD_START_MAIN << " Loaded from CTRL V: " << path;
						vec.clear();
						vec.push_back(std::string(path));
						conf.write_conf().set("general", "last_seen_list", std::vector<std::string>{ path });
						window.replace_image(std::move(fp), get_final_name(path));
					}
					else {
						auto pott = get_files_in_folder(path);

						if (pott.size()) {
							conf.write_conf().set("general", "last_seen_list", pott);
							vec.safe([&](std::vector<std::string>& vv) { vv = std::move(pott); });
							index = 0;
						}
						else {
							std::string __nam = "Could not open file or folder '" + path + "'";
							al_show_native_message_box(nullptr, "Can't open file/folder", "Failed opening file/folder from clipboard", __nam.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
							return;
						}

						if (vec.size()) {
							auto fp = make_hybrid<file>();
							if (fp->open(vec.index(0), file::open_mode_e::READ_TRY))
								window.replace_image(std::move(fp), get_final_name(vec.index(0)));
						}
					}
				}
			}
				break;
			}
		}
		else {
			switch (ev.key_id) {
			case ALLEGRO_KEY_F11:
				window.hide_menu().wait();

				window->toggle_flag(ALLEGRO_FULLSCREEN_WINDOW).wait();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				if (!window.is_fullscreen()) window.show_menu().wait(); // if not fullscreen after, show

				conf.write_conf().set("general", "was_fullscreen", window.is_fullscreen());

				window.set_update_camera();
				break;
			case ALLEGRO_KEY_R:
				window.set_camera_x(0.0f);
				window.set_camera_y(0.0f);
				window.set_camera_z(1.0f);
				window.set_camera_r(0.0f);
				break;
			case ALLEGRO_KEY_F:
			{
				float fff = window.get_camera_r() + static_cast<float>(0.5 * ALLEGRO_PI);
				if (fff >= static_cast<float>(1.99 * ALLEGRO_PI)) {
					//image_on_screen.set<float>(enum_sprite_float_e::RO_DRAW_PROJ_ROTATION, -static_cast<float>(0.5 * ALLEGRO_PI));
					fff = 0.0f;
				}
				window.set_camera_r(fff);
			}
			break;
			case ALLEGRO_KEY_ESCAPE:
			{
				int answ = al_show_native_message_box(nullptr, "Are you sure you want to quit?", "Are you sure you want to close the app?", "\"ESCAPE\" is a quick way to close this app. You can hit that and ENTER to close fast!", nullptr, ALLEGRO_MESSAGEBOX_OK_CANCEL);
				if (answ == 1) window.set_quit();
			}
				break;
			case ALLEGRO_KEY_LEFT:
			{
				size_t old_indx = index;
				if (index-- == 0) index = vec.size() - 1;
				if (old_indx == index) break;

				const auto path = vec.index(index);

				if (path.find("http") == 0) {
					auto fp = get_URL(path);
					if (fp.valid() && fp->is_open())
						window.replace_image(std::move(fp), get_final_name(path));
				}
				else {
					auto fp = make_hybrid<file>();
					if (fp->open(path, file::open_mode_e::READ_TRY))
						window.replace_image(std::move(fp), get_final_name(path));
				}
			}
				break;
			case ALLEGRO_KEY_RIGHT:
			{
				size_t old_indx = index;
				if (++index == vec.size()) index = 0;
				if (old_indx == index) break;

				const auto path = vec.index(index);

				if (path.find("http") == 0) {
					auto fp = get_URL(path);
					if (fp.valid() && fp->is_open())
						window.replace_image(std::move(fp), get_final_name(path));
				}
				else {
					auto fp = make_hybrid<file>();
					if (fp->open(path, file::open_mode_e::READ_TRY))
						window.replace_image(std::move(fp), get_final_name(path));
				}
			}
				break;
			}
		}
	});

	cout << IMGVW_STANDARD_START_MAIN << "Loading first image, if any...";

	if (vec.size()) {
		const auto path = vec.index(0);

		if (path.find("http") == 0) {
			auto fp = get_URL(path);
			if (fp.valid() && fp->is_open())
				window.replace_image(std::move(fp), get_final_name(path));
		}
		else {
			auto fp = make_hybrid<file>();
			if (fp->open(path, file::open_mode_e::READ_TRY))
				window.replace_image(std::move(fp), get_final_name(path));
		}
	}

	// check for updates here

	cout << IMGVW_STANDARD_START_MAIN << "Done loading app.";

	while (keep_rolling) std::this_thread::sleep_for(std::chrono::seconds(1));

	cout << IMGVW_STANDARD_START_MAIN << "Closing Image Viewer...";

	window.set_quit();

	return 0;
}


std::vector<std::string> parse_call(const int argc, const char* argv[])
{
	std::vector<std::string> lt;
	for (int a = 1; a < argc; a++)
	{
		std::string interp = argv[a];
		file tester;
		bool good = (interp.find("http") == 0) || tester.open(interp, file::open_mode_e::READ_TRY);
		if (good) {
			cout << IMGVW_STANDARD_START_MAIN << " Added from call: " << interp;
			lt.push_back(std::move(interp));
		}
	}
	return lt;
}

std::vector<std::string> recover_conf_files(const ImageViewer::ConfigManager& conf)
{
	std::vector<std::string> lt;
	for (const auto& each : conf.read_conf().get_array<std::string>("general", "last_seen_list"))
	{
		file tester;
		bool good = (each.find("http") == 0) || tester.open(each, file::open_mode_e::READ_TRY);
		if (good) {
			cout << IMGVW_STANDARD_START_MAIN << " Added from config: " << each;
			lt.push_back(each);
		}
	}
	return lt;
}

std::vector<std::string> get_files_in_folder(const std::string& path)
{
	const std::string opts[] = { ".jpg", ".png", ".bmp", ".jpeg", ".gif" };
	std::vector<std::string> pott;

	std::error_code err; // no except
	for (const auto& entry : std::filesystem::directory_iterator(path, std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied, err))
	{
		if (entry.is_regular_file()) {
			const std::string each = entry.path().string();

			bool good = false;
			for (const auto& known : opts) {
				size_t p = each.rfind(known);
				if (p == std::string::npos) continue;
				if (p + known.size() != each.size()) continue;
				good = true;
				break;
			}

			if (!good) continue;

			file tester;
			good = tester.open(each, file::open_mode_e::READ_TRY);
			if (!good) continue;

			cout << IMGVW_STANDARD_START_MAIN << " Added from get_files_in_folder: " << entry.path().string();
			pott.push_back(entry.path().string());
		}
	}

	return pott;
}

hybrid_memory<file> get_URL(const std::string& path)
{
	if (path.find("http") != 0) return {};

	auto fp = make_hybrid_derived<file, tempfile>();

	if (((tempfile*)fp.get())->open("imgvw_temp_XXXXXX.tmp")) {
		downloader down;
		if (!down.get_store(path, [&](const char* buf, const size_t len) { fp->write(buf, len); })) return {};

		fp->flush();
		return fp;
	}
	return {};
}

std::string get_final_name(std::string str)
{
	if (str.find("http") == 0 || str.size() < 3) return str;

	size_t p = str.rfind('/');
	if (p == std::string::npos) p = str.rfind('/');
	if (p == std::string::npos) return str;

	if (p < str.size() - 1) p++;

	return str.substr(p);
}