#include <Lunaris/all.h>

#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_windows.h>

#include <nlohmann/json.hpp>

#include "resource.h"
#include <shellapi.h>
#include <filesystem>

using namespace Lunaris;

std::string gen_path();

const std::string url_update_check = "https://api.github.com/repos/Lohkdesgds/ImageViewer/releases/latest";
const std::string common_path = gen_path();
const std::string version_str = "v2.2.0";
const std::string fixed_app_name = "ImageViewer " + version_str + " | Lunaris edition 2021";
constexpr size_t max_timeouts = 3;
const double event_generic_time_relax = 1.0; // sec
const double event_display_time_relax = 0.2; // sec
const double show_text_on_screen_for = 2.0; // sec

struct auxiliar_data {
	std::atomic<unsigned> move_buttons_triggered = 0; // mouse, keyboard SPACE
	bool quit = false;
	bool got_into_load = false;
	std::atomic<bool> can_quit_fast = false;
	size_t times_no_quit = 0;
	std::atomic<bool> update_camera = true;
	std::atomic<float> zoom = 1.0f;
	//std::atomic<float> camera_offx = 0.0f;
	//std::atomic<float> camera_offy = 0.0f;
	std::atomic<size_t> index = 0;

	int mouse_fix_x = 0;
	int mouse_fix_y = 0;

	bool keyboard_space_triggered = false;
	bool mouse_0_triggered = false;
	std::atomic<int> update_indexed_image = 0; // off
};

void icon_fix(const display&);
void check_update_async(auxiliar_data&, config&);

int main(int argc, char* argv[]) {

	cout << console::color::AQUA << "Loading ImageViewer " << version_str << " running on Lunaris Build #" << LUNARIS_BUILD_NUMBER;

	generic_event_handler handling;
	display disp;	
	std::vector<std::string> parsed;
	config conf;
	auxiliar_data aux_data;
	transform camera, camera_one_o_one;
	ALLEGRO_DISPLAY* source_disp = nullptr;
	thread error_check, update_check;
	double last_event = 0.0;

	hybrid_memory<texture> file_texture;
	hybrid_memory<texture> file_texture_gif;
	hybrid_memory<font> embedded_font;

	block image_on_screen;
	text name_on_screen;

	const color black = color(0.0f, 0.0f, 0.0f, 0.2f);

	error_check.task_async([&] {
		if (aux_data.quit || aux_data.got_into_load) {
			if (aux_data.times_no_quit < max_timeouts) {
				++aux_data.times_no_quit;
			}
			else {
				disp.set_window_title(fixed_app_name + " | I'm still trying to load... (ESCAPE/X = abort)");
				aux_data.can_quit_fast = true;
			}
		}
		else if (aux_data.times_no_quit > 0) {
			aux_data.times_no_quit = 0;
			aux_data.can_quit_fast = false;
			disp.set_window_title(fixed_app_name);
		}
	}, thread::speed::INTERVAL, 2.0);


	const auto func_update_camera = [&] {
		float prop = 1.0f;
		if (!file_texture.empty() && !file_texture->empty()) {
			const double pp = fabs(cos(image_on_screen.get<float>(enum_sprite_float_e::ROTATION))); // 1.0 for 0 or 180
			if (pp > 0.9) prop = 1.0f * file_texture->get_width() / file_texture->get_height();
			else prop = 1.0f * file_texture->get_height() / file_texture->get_width();
		}
		else if (!file_texture_gif.empty() && !file_texture_gif->empty()) {
			const double pp = fabs(cos(image_on_screen.get<float>(enum_sprite_float_e::ROTATION))); // 1.0 for 0 or 180
			if (pp > 0.9) prop = 1.0f * file_texture_gif->get_width() / file_texture_gif->get_height();
			else prop = 1.0f * file_texture_gif->get_height() / file_texture_gif->get_width();
		}
		else return;

		camera.build_classic_fixed_proportion(disp.get_width(), disp.get_height(), prop, aux_data.zoom);
		camera_one_o_one.build_classic_fixed_proportion(disp.get_width(), disp.get_height(), 1.0f, 1.0f);
		camera.apply();
	};

	const auto quit_fast_f = [&] {
		al_show_native_message_box(nullptr, "Sorry about that", "I couldn't load in time!", "Sorry about taking so long to open this file. Maybe something bad happened. Please report!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		std::terminate();
	};


	if (!conf.load(common_path + "last.conf")) conf.save_path(common_path + "last.conf");
	conf.ensure("general", "was_fullscreen", false, config::config_section_mode::SAVE);
	conf.ensure("general", "smooth_scale", true, config::config_section_mode::SAVE);
	conf.ensure("general", "width", 1280, config::config_section_mode::SAVE);
	conf.ensure("general", "height", 720, config::config_section_mode::SAVE);
	conf.ensure("general", "last_version_check", version_str, config::config_section_mode::SAVE);
	conf.ensure<std::string>("general", "last_seen_list", {}, config::config_section_mode::SAVE);

	cout << console::color::GREEN << "Working on it...";

	update_check.task_async([&] {check_update_async(aux_data, conf); }, thread::speed::ONCE);

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
		.set_window_title(fixed_app_name)
		.set_fullscreen(conf.get_as<bool>("general", "was_fullscreen"))
		.set_extra_flags(al_get_new_display_flags() | ALLEGRO_RESIZABLE | (conf.get_as<bool>("general", "was_fullscreen") ? ALLEGRO_FULLSCREEN_WINDOW : 0))
	)) {
		cout << console::color::RED << "Bad news. Could not create display.";
		al_show_native_message_box(nullptr, "Trouble creating Display", "Can't create display!", "Please check you drivers or ask for support!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		return -1;
	}

	if (!(source_disp = disp.get_raw_display())) {
		cout << console::color::RED << "Bad news. Could not find display.";
		disp.destroy();
		al_show_native_message_box(nullptr, "Trouble finding Display", "Can't find created display!", "Please check you drivers or ask for support!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		return -1;
	}

	icon_fix(disp);

	file_texture = make_hybrid<texture>();
	file_texture_gif = make_hybrid_derived<texture, texture_gif>();

	disp.hook_event_handler([&](const ALLEGRO_EVENT& ev) {
		last_event = al_get_time() + event_display_time_relax;

		switch (ev.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			aux_data.quit = true;
			if (aux_data.can_quit_fast) {
				quit_fast_f();
			}
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			conf.set("general", "width", ev.display.width);
			conf.set("general", "height", ev.display.height);
			aux_data.update_camera = true;
			break;
		}
	});

	handling.hook_event_handler([&](const ALLEGRO_EVENT& ev) {
		last_event = al_get_time() + event_generic_time_relax;

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
					image_on_screen.set<float>(enum_sprite_float_e::ROTATION, 0.0f);
					aux_data.zoom = 1.0f;
					aux_data.update_camera = true;
					break;
				case ALLEGRO_KEY_F:
				{
					float fff = image_on_screen.get<float>(enum_sprite_float_e::ROTATION) + static_cast<float>(0.5 * ALLEGRO_PI);
					if (fff >= static_cast<float>(1.99 * ALLEGRO_PI)) {
						image_on_screen.set<float>(enum_sprite_float_e::RO_DRAW_PROJ_ROTATION, -static_cast<float>(0.5 * ALLEGRO_PI));
						fff = 0.0f;
					}
					image_on_screen.set<float>(enum_sprite_float_e::ROTATION, fff);
					aux_data.update_camera = true;
				}
					break;
				case ALLEGRO_KEY_ESCAPE:
					aux_data.quit = true;
					if (aux_data.can_quit_fast) {
						quit_fast_f();
					}
					break;
				case ALLEGRO_KEY_LEFT:
					image_on_screen.set<float>(enum_sprite_float_e::POS_X, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::POS_Y, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::ROTATION, 0.0f);
					aux_data.zoom = 1.0f;
					aux_data.update_indexed_image--;
					aux_data.update_camera = true;
					break;
				case ALLEGRO_KEY_RIGHT:
					image_on_screen.set<float>(enum_sprite_float_e::POS_X, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::POS_Y, 0.0f);
					image_on_screen.set<float>(enum_sprite_float_e::ROTATION, 0.0f);
					aux_data.zoom = 1.0f;
					aux_data.update_indexed_image++;
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

	embedded_font = make_hybrid<font>();
	if (!embedded_font->create_builtin_font()) {
		cout << console::color::RED << "Bad news. Could not create font.";
		al_show_native_message_box(nullptr, "Trouble creating Font!", "Can't create font!", "This is awkward! I've never seen this issue before! Please report!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		disp.destroy();
		return -1;
	}

	name_on_screen.font_set(embedded_font);
	name_on_screen.set<float>(enum_sprite_float_e::POS_X, -0.99f);
	name_on_screen.set<float>(enum_sprite_float_e::POS_Y,  0.89f);
	name_on_screen.set<float>(enum_sprite_float_e::SCALE_G, 0.1f);
	name_on_screen.set<float>(enum_sprite_float_e::SCALE_X, 0.4f);
	name_on_screen.set<int>(enum_text_integer_e::DRAW_ALIGNMENT, ALLEGRO_ALIGN_LEFT);
	name_on_screen.set<bool>(enum_sprite_boolean_e::DRAW_TRANSFORM_COORDS_KEEP_SCALE, true);
	name_on_screen.set<safe_data<std::string>>(enum_text_safe_string_e::STRING, std::string("Processing..."));
	//for (int __c = 1; 255 - 25 * __c > 0; __c++) {
	//	int ctee = (255 - 25 * __c);
	//	name_on_screen.shadow_insert(text_shadow(0.0015f * __c, 0.014f * __c, color(ctee / 10, ctee / 10, ctee / 10, ctee)));
	//}

	last_event = al_get_time() + event_display_time_relax;

	while (!aux_data.quit) 
	{
		if (last_event < al_get_time()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		if (aux_data.update_indexed_image != 0 || image_on_screen.texture_size() == 0) {
			
			aux_data.got_into_load = true;

			image_on_screen.texture_remove_all();
			name_on_screen.shadow_remove_all();
			name_on_screen.set<color>(enum_sprite_color_e::DRAW_TINT, color(0.8f, 0.8f, 0.8f, 1.0f));
			name_on_screen.set<safe_data<std::string>>(enum_text_safe_string_e::STRING, std::string("Loading..."));
			name_on_screen.shadow_insert(text_shadow(0.003f, 0.015f, color(0.0f, 0.0f, 0.0f, 0.9f)));
			if ((!file_texture.empty() && !file_texture->empty()) || (!file_texture_gif.empty() && !file_texture_gif->empty())) camera_one_o_one.apply();

			thread parallel_load([&] {
				{
					long long cpy_ref = static_cast<long long>(aux_data.update_indexed_image);
					aux_data.update_indexed_image = 0;

					if (cpy_ref < 0 && static_cast<size_t>(-cpy_ref) > parsed.size()) cpy_ref = 0;
					if (cpy_ref > 0 && static_cast<size_t>(cpy_ref) > parsed.size()) cpy_ref = 0;

					size_t res = static_cast<size_t>((cpy_ref % static_cast<long long>(parsed.size())) + static_cast<long long>(parsed.size()));
					aux_data.index = (res + aux_data.index) % parsed.size();
				}

				size_t old_p = aux_data.index;
				int loops = 0;

				while (!aux_data.quit) {
					name_on_screen.set<safe_data<std::string>>(enum_text_safe_string_e::STRING, std::string("Trying to load file #") + std::to_string(aux_data.index) + "...");

					if (old_p == aux_data.index && ++loops > 2) {
						al_show_native_message_box(nullptr, "Could not load image(s)!", "Something went wrong", "Maybe the file format is not supported? I tried all options you gave me. None worked.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
						aux_data.quit = true;
						break;
					}

					const std::string& enstr = parsed[aux_data.index];
					bool is_gif = false;
					if (enstr.size() > 4) {
						const std::string ending = ".gif";
						is_gif = std::equal(ending.rbegin(), ending.rend(), enstr.rbegin());
					}

					cout << "Loading '" << enstr << "'";

					if (is_gif) {
						file_texture->destroy();

						auto gif_interf = (texture_gif*)file_texture_gif.get();
						if (!gif_interf->load(enstr)) aux_data.index = (++aux_data.index >= parsed.size()) ? 0ULL : static_cast<unsigned long long>(aux_data.index);
						else {
							image_on_screen.texture_insert(file_texture_gif);
							break;
						}
					}
					else {
						file_texture_gif->destroy();

						if (!file_texture->load(enstr)) aux_data.index = (++aux_data.index >= parsed.size()) ? 0ULL : static_cast<unsigned long long>(aux_data.index);
						else {
							image_on_screen.texture_insert(file_texture);
							break;
						}
					}
				}

				//aux_data.index = aux_data.index;
				aux_data.got_into_load = false;
			}, thread::speed::ONCE);

			double rn_t = al_get_time();
			const float animation_transparency_load = 0.12f;
			const float animation_transparency_posload = 0.35f;
			const double min_animation_time = 0.2;

			while (aux_data.got_into_load && !aux_data.quit) {
				auto time_wait = std::chrono::system_clock::now() + std::chrono::milliseconds(33);
				color rndcolor(
					(0.25f + 0.2f * static_cast<float>(cos((al_get_time() - rn_t) * 1.88742))) * animation_transparency_load,
					(0.25f + 0.2f * static_cast<float>(cos((al_get_time() - rn_t) * 2.59827))) * animation_transparency_load,
					(0.25f + 0.2f * static_cast<float>(cos((al_get_time() - rn_t) * 1.04108))) * animation_transparency_load,
					animation_transparency_load
				);
				al_draw_filled_rectangle(-9999.0f, -9999.0f, 19998.0f, 19998.0f, rndcolor);

				name_on_screen.draw();

				disp.flip();
				std::this_thread::sleep_until(time_wait);
			}

			if (aux_data.quit) continue;

			//al_convert_bitmaps();
			func_update_camera();
			aux_data.update_camera = false;

			{
				image_on_screen.set<color>(enum_sprite_color_e::DRAW_TINT, color(animation_transparency_posload, animation_transparency_posload, animation_transparency_posload, animation_transparency_posload));
				image_on_screen.set<bool>(enum_sprite_boolean_e::DRAW_USE_COLOR, true);

				for (const double tt = al_get_time() + min_animation_time; tt > al_get_time();)
				{
					auto time_wait = std::chrono::system_clock::now() + std::chrono::milliseconds(33);
					image_on_screen.draw();
					disp.flip();
					std::this_thread::sleep_until(time_wait);
				}

				image_on_screen.set<bool>(enum_sprite_boolean_e::DRAW_USE_COLOR, false);
			}
			
			last_event = al_get_time() + event_display_time_relax;
			{
				std::string aaa = parsed[aux_data.index];
				if (aaa.size() > 80) aaa = "..." + aaa.substr(aaa.size() - 77, aaa.size());
				name_on_screen.set<safe_data<std::string>>(enum_text_safe_string_e::STRING, std::string("[#") + std::to_string(aux_data.index) + "] " + aaa);
			}

		}
		if (aux_data.update_camera) 
		{
			aux_data.update_camera = false;
			func_update_camera();
		}

		//black.clear_to_this();
		al_draw_filled_rectangle(-9999.9f, -9999.9f, 9999.9f, 9999.9f, black);

		if (image_on_screen.texture_size()) image_on_screen.draw();
		else quit_fast_f();

		if (auto timm = ((last_event + show_text_on_screen_for) - al_get_time()); timm > 0.0) {
			timm *= 1.2;
			if (timm > 1.0) timm = 1.0;

			name_on_screen.set<color>(enum_sprite_color_e::DRAW_TINT, color(static_cast<float>(timm) * 0.8f, static_cast<float>(timm) * 0.8f, static_cast<float>(timm) * 0.8f, static_cast<float>(timm)));
			name_on_screen.shadow_remove_all();
			name_on_screen.shadow_insert(text_shadow(0.003f, 0.015f, color(0.0f, 0.0f, 0.0f, 0.9f * static_cast<float>(timm))));

			camera_one_o_one.apply();
			name_on_screen.draw();
			camera.apply();
		}

		disp.flip();
	}

	disp.destroy();
	conf.flush();

	error_check.signal_stop();
	update_check.signal_stop();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	error_check.force_kill();
	update_check.force_kill();
}

std::string gen_path()
{
	al_init();
	auto path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
	std::string path_str = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);
	if (path_str.empty()) {
		al_show_native_message_box(nullptr, "Early load error", "Can't find config path!", "Somehow I can't find app data path! Please report!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		throw std::runtime_error("FATAL ERROR CANNOT FIND APPDATA PATH!");
	}
#ifdef _WIN32
	path_str.pop_back();
	size_t p = path_str.rfind(ALLEGRO_NATIVE_PATH_SEP);
	if (p == std::string::npos) {
		al_show_native_message_box(nullptr, "Early load error", "Can't find config path!", "Somehow the data path I found is invalid! Please report!", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
		throw std::runtime_error("FATAL ERROR CANNOT FIND APPDATA PATH!");
	}
	path_str = path_str.substr(0, p + 1);
#endif
	al_destroy_path(path);

	path_str += std::string("Lohk's Studios") + ALLEGRO_NATIVE_PATH_SEP + std::string("Image Viewer") + ALLEGRO_NATIVE_PATH_SEP;

	for (size_t p = 0; p < path_str.size(); p++) {
		size_t newp = path_str.find(ALLEGRO_NATIVE_PATH_SEP, p);
		if (newp < p) break; // find out of range?
		else p = newp;

		std::string tmp = path_str.substr(0, p);

		std::error_code err;
		if (!std::filesystem::create_directories(tmp, err) && err.value() != 0) {
			cout << console::color::GOLD << "Create directories error:" << err.message();
		}

	}

	return path_str;
}

void icon_fix(const display& disp)
{
	HWND winhandle;
	HICON icon;

	icon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR); //LoadIconA(GetModuleHandle(NULL), "IDI_ICON1");
	if (icon) {
		winhandle = al_get_win_window_handle(disp.get_raw_display());
		SetClassLongPtr(winhandle, GCLP_HICON, (LONG_PTR)icon);
		SetClassLongPtr(winhandle, GCLP_HICONSM, (LONG_PTR)icon);
	}
}

void check_update_async(auxiliar_data& aux, config& conf)
{
	cout << console::color::DARK_PURPLE << "Checking version...";

	downloader down;
	if (!down.get(url_update_check)) {
		cout << console::color::DARK_PURPLE << "Failed to check version.";
		return;
	}

	nlohmann::json gitt = nlohmann::json::parse(down.read(), nullptr, false);

	if (gitt.empty() || gitt.is_discarded()) {
		cout << console::color::DARK_PURPLE << "Failed to check version.";
		return;
	}

	std::string update_url, tag_name, title, body;

	if (auto it = gitt.find("html_url"); it != gitt.end() && it->is_string()) update_url = it->get<std::string>();
	if (auto it = gitt.find("tag_name"); it != gitt.end() && it->is_string()) tag_name = it->get<std::string>();
	if (auto it = gitt.find("name"); it != gitt.end() && it->is_string())     title = it->get<std::string>();
	if (auto it = gitt.find("body"); it != gitt.end() && it->is_string())     body = it->get<std::string>();

	if (update_url.empty() || tag_name.empty() || title.empty() || body.empty()) {
		cout << console::color::DARK_PURPLE << "Invalid new version body. Skipped.";
		return;
	}

	if (conf.get("general", "last_version_check") != tag_name && version_str != tag_name) {
#ifdef _DEBUG
		cout << console::color::DARK_PURPLE << "New version! But this is DEBUG, so skip (no save).";
		return;
#else
		cout << console::color::DARK_PURPLE << "New version!";
		std::this_thread::sleep_for(std::chrono::seconds(3));

		int opt = al_show_native_message_box(nullptr, ("New version detected! (" + tag_name + ") Download (OK) or skip this version (Cancel)?").c_str(), title.c_str(), body.c_str(), nullptr, ALLEGRO_MESSAGEBOX_OK_CANCEL);

		if (opt == 1) { // ask again if user don't properly update
			ShellExecuteA(0, 0, update_url.c_str(), 0, 0, SW_SHOW);
			return;
		}
#endif
	}
	else {
		cout << console::color::DARK_PURPLE << "No new version [Current: " << version_str << ", Latest: " << tag_name << ", Last notified: " << conf.get("general", "last_version_check") << "]";
	}

	conf.set("general", "last_version_check", tag_name);
}