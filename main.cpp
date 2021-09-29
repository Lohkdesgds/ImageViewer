#include <Lunaris/graphics.h>
#include <Lunaris/utility.h>
#include <Lunaris/events.h>

#include <allegro5/allegro_native_dialog.h>

using namespace Lunaris;

constexpr int minimum_scale = -45;
constexpr int maximum_scale = 1000;

std::string gen_path();

const std::string common_path = gen_path();

struct auxiliar_data {
	std::atomic<unsigned> move_buttons_triggered = 0; // mouse, keyboard SPACE
	bool quit = false;
	std::atomic<bool> update_camera = true;
	std::atomic<float> zoom = 1.0f;
	//std::atomic<float> camera_offx = 0.0f;
	//std::atomic<float> camera_offy = 0.0f;
	std::atomic<size_t> index = 0;

	int mouse_fix_x = 0;
	int mouse_fix_y = 0;

	bool keyboard_space_triggered = false;
	bool mouse_0_triggered = false;
	bool update_indexed_image = true; // update once
};


int main(int argc, char* argv[]) {

	generic_event_handler handling;
	display disp;	
	std::vector<std::string> parsed;
	config conf;
	auxiliar_data aux_data;
	transform camera;
	ALLEGRO_DISPLAY* source_disp = nullptr;

	auto file_texture = make_hybrid<texture>();
	block image_on_screen;
	const color black = color(0, 0, 0);
	const color loading = color(127, 127, 127);

	const auto func_update_camera = [&] {
		float prop = 1.0f;
		if (!file_texture.empty()) {
			prop = 1.0f * file_texture->get_width() / file_texture->get_height();
		}

		camera.build_classic_fixed_proportion(disp.get_width(), disp.get_height(), prop, aux_data.zoom);
		//camera.translate(aux_data.camera_offx, aux_data.camera_offy);
		camera.apply();
	};


	if (!conf.load(common_path + "last.conf")) conf.save_path(common_path + "last.conf");
	conf.ensure("general", "was_fullscreen", false, config::config_section_mode::SAVE);
	conf.ensure("general", "smooth_scale", true, config::config_section_mode::SAVE);
	conf.ensure("general", "width", 1280, config::config_section_mode::SAVE);
	conf.ensure("general", "height", 720, config::config_section_mode::SAVE);
	conf.ensure<std::string>("general", "last_seen_list", {}, config::config_section_mode::SAVE);

	cout << console::color::GREEN << "Loading ImageViewer...";

	for (int u = 1; u < argc; u++) {
		file tempfp;
		if (!tempfp.open(argv[u], "rb") || tempfp.size() <= 0) {
			cout << console::color::YELLOW << "Failed to load " << argv[u] << " from arguments list.";
		}
		else {
			cout << console::color::GREEN << "Added '" << argv[u] << "' to the list";
			parsed.push_back(argv[u]);
		}
	}

	if (!parsed.size()) {
		cout << "No file in arguments list, so loading history...";
		auto last_list = conf.get_array<std::string>("general", "last_seen_list");
		parsed = std::move(last_list);

		for (size_t u = 0; u < parsed.size(); u++) {
			file tempfp;
			if (!tempfp.open(parsed[u], "rb") || tempfp.size() <= 0) {
				cout << console::color::YELLOW << "Failed to load " << argv[u] << " from history list.";
				parsed.erase(parsed.begin() + u--);
			}
			else {
				cout << console::color::GREEN << "Added '" << parsed[u] << "' to the list";
			}
		}

		if (!parsed.size()) {
			cout << console::color::RED << "Not enough arguments or invalid ones. Please try dragging and dropping the image file onto the app.";
			
			al_show_native_message_box(nullptr, "Not enough arguments", "Empty queue!", "No files are saved/valid in history or call. Please try again.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
#ifdef _DEBUG
			std::this_thread::sleep_for(std::chrono::seconds(10));
#endif
			return 1;
		}
	}

	conf.set("general", "last_seen_list", parsed);

	al_set_new_bitmap_flags(al_get_new_bitmap_flags() | (conf.get_as<bool>("general", "smooth_scale") ? (ALLEGRO_MAG_LINEAR | ALLEGRO_MIN_LINEAR) : 0));
	al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);

	if (!disp.create(
		display_config()
		.set_display_mode(
			display_options()
			.set_width(conf.get_as<int>("general", "width"))
			.set_height(conf.get_as<int>("general", "height"))
		)
		.set_vsync(true)
		.set_window_title("ImageViewer | Lunaris edition 2021")
		.set_fullscreen(conf.get_as<bool>("general", "was_fullscreen"))
		.set_extra_flags(al_get_new_display_flags() | ALLEGRO_RESIZABLE | (conf.get_as<bool>("general", "was_fullscreen") ? ALLEGRO_FULLSCREEN_WINDOW : 0))
	)) {
		cout << console::color::RED << "Bad news. Could not create display.";
		al_show_native_message_box(nullptr, "Trouble creating Display", "Can't create display!", "Please check you drivers or ask for support!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		return -1;
	}

	if (!(source_disp = al_get_current_display())) {
		cout << console::color::RED << "Bad news. Could not find display.";
		disp.destroy();
		al_show_native_message_box(nullptr, "Trouble finding Display", "Can't find created display!", "Please check you drivers or ask for support!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		return -1;
	}

	disp.hook_event_handler([&](const ALLEGRO_EVENT& ev) {
		switch (ev.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			aux_data.quit = true;
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			conf.set("general", "width", ev.display.width);
			conf.set("general", "height", ev.display.height);
			aux_data.update_camera = true;
			break;
		}
	});

	handling.hook_event_handler([&](const ALLEGRO_EVENT& ev) {
		switch (ev.type) {
		case ALLEGRO_EVENT_KEY_DOWN:
			if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
				if (!aux_data.keyboard_space_triggered) {
					++aux_data.move_buttons_triggered;
				}
				aux_data.keyboard_space_triggered = true;
			}
			else {
				switch (ev.keyboard.keycode) {
				case ALLEGRO_KEY_F11:
					disp.toggle_flag(ALLEGRO_FULLSCREEN_WINDOW);
					conf.set("general", "was_fullscreen", disp.get_flags() & ALLEGRO_FULLSCREEN_WINDOW);
					aux_data.update_camera = true;
					break;
				case ALLEGRO_KEY_R:
					image_on_screen.set<float>(enum_sprite_float_e::POS_X, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::POS_Y, 0.0f);
					aux_data.zoom = 1.0f;
					aux_data.update_camera = true;
					break;
				case ALLEGRO_KEY_ESCAPE:
					aux_data.quit = true;
					break;
				case ALLEGRO_KEY_LEFT:
					if (aux_data.index-- == 0) aux_data.index = parsed.size() - 1;
					image_on_screen.set<float>(enum_sprite_float_e::POS_X, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::POS_Y, 0.0f);
					aux_data.zoom = 1.0f;
					aux_data.update_indexed_image = true;
					aux_data.update_camera = true;
					break;
				case ALLEGRO_KEY_RIGHT:
					if (++aux_data.index >= parsed.size()) aux_data.index = 0;
					image_on_screen.set<float>(enum_sprite_float_e::POS_X, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::POS_Y, 0.0f);
					aux_data.zoom = 1.0f;
					aux_data.update_indexed_image = true;
					aux_data.update_camera = true;
					break;
				}
			}
			break;
		case ALLEGRO_EVENT_KEY_UP:
			if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
				if (aux_data.keyboard_space_triggered) {
					--aux_data.move_buttons_triggered;
				}
				aux_data.keyboard_space_triggered = false;
			}
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
		{
			if (ev.mouse.button == 1) {
				if (!aux_data.mouse_0_triggered) {
					++aux_data.move_buttons_triggered;
				}
				aux_data.mouse_0_triggered = true;
			}
		}
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
		{
			if (ev.mouse.button == 1) {
				if (aux_data.mouse_0_triggered) {
					--aux_data.move_buttons_triggered;
				}
				aux_data.mouse_0_triggered = false;
			}
		}
			break;
		case ALLEGRO_EVENT_MOUSE_AXES:
		{
			bool any_news = false;
			if (aux_data.move_buttons_triggered > 0) {

				image_on_screen.set<float>(enum_sprite_float_e::POS_X, image_on_screen.get<float>(enum_sprite_float_e::POS_X) + 0.0005f * ev.mouse.dx / aux_data.zoom);
				image_on_screen.set<float>(enum_sprite_float_e::POS_Y, image_on_screen.get<float>(enum_sprite_float_e::POS_Y) + 0.0005f * ev.mouse.dy / aux_data.zoom);

				//aux_data.camera_offx = aux_data.camera_offx + 0.3f * ev.mouse.dx; // can't +=? lol
				//aux_data.camera_offy = aux_data.camera_offy + 0.3f * ev.mouse.dy; // can't +=? lol

				any_news = true;

				al_set_mouse_xy(source_disp, aux_data.mouse_fix_x, aux_data.mouse_fix_y);
			}
			else {
				aux_data.mouse_fix_x = ev.mouse.x;
				aux_data.mouse_fix_y = ev.mouse.y;
			}

			if (ev.mouse.dz != 0) {
				const float rn = aux_data.zoom;
				if (rn < 0.25f) {
					const float val = rn + ev.mouse.dz * 0.0125f;
					if (val > 0.0f) aux_data.zoom = val;
				}
				else if (rn < 0.5f) {
					aux_data.zoom = rn + ev.mouse.dz * 0.05f;
				}
				else if (rn < 1.5f) {
					aux_data.zoom = rn + ev.mouse.dz * 0.10f;
				}
				else if (rn < 5.0f) {
					aux_data.zoom = rn + ev.mouse.dz * 0.25f;
				}
				else if (rn < 25.0f) {
					aux_data.zoom = rn + ev.mouse.dz * 1.0f;
				}
				else {
					aux_data.zoom = rn + ev.mouse.dz * 10.0f;
				}
				any_news = true;
			}

			if (any_news) aux_data.update_camera = true;
		}
			break;
		}

		if (aux_data.move_buttons_triggered == 0) {
			al_show_mouse_cursor(source_disp);
		}
		else {
			al_hide_mouse_cursor(source_disp);
		}
	});

	handling.install_keyboard();
	handling.install_mouse();

	image_on_screen.set<float>(enum_sprite_float_e::SCALE_G, 2.0f);
	image_on_screen.set<float>(enum_sprite_float_e::DRAW_MOVEMENT_RESPONSIVENESS, 0.4f);
	image_on_screen.texture_insert(file_texture);

	while (!aux_data.quit) 
	{
		black.clear_to_this();

		if (aux_data.update_indexed_image) {
			aux_data.update_indexed_image = false;

			loading.clear_to_this();
			disp.flip();

			while (!file_texture->load(parsed[aux_data.index]) && !aux_data.quit) aux_data.index = (++aux_data.index >= parsed.size()) ? 0ULL : static_cast<unsigned long long>(aux_data.index);
			if (aux_data.quit) continue;
		}
		if (aux_data.update_camera) 
		{
			aux_data.update_camera = false;
			func_update_camera();
		}

		image_on_screen.draw();

		disp.flip();
	}

	conf.flush();
}

std::string gen_path()
{
	al_init();
	auto path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
	std::string path_str = al_path_cstr(path, '/');
	if (path_str.empty()) {
		al_show_native_message_box(nullptr, "Early load error", "Can't find config path!", "Somehow I can't find app data path! Please report!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		throw std::runtime_error("FATAL ERROR CANNOT FIND APPDATA PATH!");
	}
#ifdef _WIN32
	path_str.pop_back();
	size_t p = path_str.rfind('/');
	if (p == std::string::npos) {
		al_show_native_message_box(nullptr, "Early load error", "Can't find config path!", "Somehow the data path I found is invalid! Please report!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		throw std::runtime_error("FATAL ERROR CANNOT FIND APPDATA PATH!");
	}
	path_str = path_str.substr(0, p + 1);
#endif
	path_str += "Lohk's Studios/Image Viewer/";
	al_destroy_path(path);
	return path_str;
}