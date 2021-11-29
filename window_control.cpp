#include "window_control.h"

namespace ImageViewer {

	void WindowControl::new_version_data::update()
	{
		cout << IMGVW_STANDARD_START_WINDOWCTL << "Checking version...";

		downloader down;
		if (!down.get(url_update_check)) {
			cout << IMGVW_STANDARD_START_WINDOWCTL << "Failed to check version.";
			return;
		}

		nlohmann::json gitt = nlohmann::json::parse(down.read(), nullptr, false);

		if (gitt.empty() || gitt.is_discarded()) {
			cout << IMGVW_STANDARD_START_WINDOWCTL << "Failed to check version.";
			return;
		}

		if (auto it = gitt.find("tag_name"); it != gitt.end() && it->is_string()) tag_name = it->get<std::string>();

		if (tag_name.empty()) {
			cout << IMGVW_STANDARD_START_WINDOWCTL << "Invalid new version body. Skipped.";
			return;
		}

		has_update = (versioning != tag_name);
		got_data = true;
	}

	void WindowControl::new_version_data::load(const config& c)
	{
		got_data = true;
		has_update = c.get_as<bool>("general", "had_update_last_time"); // has_update
		tag_name = c.get("general", "last_version_check"); // tag_name
	}

	void WindowControl::new_version_data::store(config& c)
	{
		c.set("general", "had_update_last_time", has_update); // has_update
		c.set("general", "last_version_check", tag_name); // tag_name
	}

	void WindowControl::throw_async(const std::function<void(void)> f)
	{
		if (!f) return;
		if (__async_task.valid()) __async_task.get();
		__async_task = std::async(std::launch::async, f);
	}

	void WindowControl::update_fps_limiters()
	{
		if (!last_was_gif && (al_get_time() - last_update_ext > deep_sleep_time))
		{
			if (disp.get_fps_limit() != deep_sleep_fps)
				disp.set_fps_limit(deep_sleep_fps);
		}
		else if (al_get_time() - last_update_ext > time_sleep)
		{
			if (disp.get_fps_limit() != disp.get_economy_fps())
				disp.set_fps_limit(disp.get_economy_fps());
		}
		else if (disp.get_fps_limit() != found_max_fps_of_computer)
		{
			disp.set_fps_limit(found_max_fps_of_computer);
		}
	}

	void WindowControl::do_draw()
	{
		if (update_camera && current_texture.valid() && !current_texture->empty()) {
			transform easy;
			const float formul = current_texture->get_width() * 1.0f / current_texture->get_height();
			easy.build_classic_fixed_proportion_auto(formul, camera_zoom);
			easy.translate(-disp.get_width() * 0.5f, -disp.get_height() * 0.5f);
			easy.rotate(camera_rot);
			easy.translate(disp.get_width() * 0.5f, disp.get_height() * 0.5f);
			easy.apply();

			update_camera = false;
		}

		switch (curr_mode) {
		case display_mode::LOADING:
		{
			if (disp.get_fps_limit() != loading_fps) {
				disp.set_fps_limit(loading_fps);
			}

			float curr_transp = animation_transparency_load + static_cast<float>(0.003 * al_get_time());
			if (curr_transp > 0.05f) curr_transp = 0.05f;

			color rndcolor(
				(0.25f + 0.2f * static_cast<float>(cos((al_get_time() - mode_change_time) * 1.88742))) * curr_transp,
				(0.25f + 0.2f * static_cast<float>(cos((al_get_time() - mode_change_time) * 2.59827))) * curr_transp,
				(0.25f + 0.2f * static_cast<float>(cos((al_get_time() - mode_change_time) * 1.04108))) * curr_transp,
				curr_transp
			);
			al_draw_filled_rectangle(-9999.0f, -9999.0f, 19998.0f, 19998.0f, rndcolor);
		}
			break;
		case display_mode::SHOW:
		{
			update_fps_limiters();
			color(0, 0, 0).clear_to_this();
			texture_show_fixed.draw();
		}
			break;
		}
	}

	void WindowControl::do_events(display_event& ev)
	{
		switch (ev.get_type()) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			throw_async([this] { destroy(); });
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			conf.write_conf().set("general", "width", ev.as_display().width);
			conf.write_conf().set("general", "height", ev.as_display().height);
			update_camera = true;
			break;
		}


		events_handlr.csafe([&ev](const std::function<void(display_event&)> f) {
			if (f) f(ev);
		});
	}

	void WindowControl::do_menu(menu_event& ev)
	{
		switch (ev.get_id()) {
		case static_cast<uint16_t>(event_menu::FILE_EXIT):
			throw_async([this] { destroy(); });
			break;
		case static_cast<uint16_t>(WindowControl::event_menu::UPDATE_VERSION):
			throw_async([this] {
				ShellExecuteA(0, 0, url_update_popup.c_str(), 0, 0, SW_SHOW);
			});
			break;
		}

		menu_handlr.csafe([&ev](const std::function<void(menu_event&)> f) {
			if (f) f(ev);
		});
	}

	void WindowControl::reset_camera()
	{
		set_camera_x(0.0f);
		set_camera_y(0.0f);
		set_camera_z(1.0f);
		set_camera_r(0.0f);
		update_camera = true;
	}

	void WindowControl::destroy()
	{
		conf.write_conf().flush();
		disp.post_task([this] {disp.destroy(); return true;});
	}

	void WindowControl::setup_menu()
	{
		raw_menu.push({
			menu_each_menu()
				.set_name("File")
				.set_id(static_cast<uint16_t>(WindowControl::event_menu::FILE))
				.push(menu_each_default()
					.set_name("Open")
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::FILE_OPEN))
				)
				.push(menu_each_default()
					.set_name("Open folder")
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::FILE_FOLDER_OPEN))
				)
				.push(menu_each_default()
					.set_name("Open from clipboard (file, folder or URL)")
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::FILE_PASTE_URL))
				)
				.push(menu_each_empty()
				)
				.push(menu_each_default()
					.set_name("Exit")
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::FILE_EXIT))
				),
			menu_each_menu()
				.set_name("Checking for updates...")
				.set_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE))
				.push(menu_each_default()
					.set_name("Open releases page")
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::UPDATE_VERSION))
				)
				.push(menu_each_empty()
				)
				.push(menu_each_default()
					.set_name("Current version: " + versioning)
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::CURRENT_VERSION))
					.set_flags(ALLEGRO_MENU_ITEM_DISABLED)
				)
				.push(menu_each_default()
					.set_name(std::string(u8"Build date: ") + __DATE__)
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::RELEASE_DATE))
					.set_flags(ALLEGRO_MENU_ITEM_DISABLED)
				)
				.push(menu_each_empty()
				)
				.push(menu_each_default()
					.set_name("Using " + std::string(LUNARIS_VERSION_SHORT))
					.set_id(static_cast<uint16_t>(WindowControl::event_menu::ABOUT_LUNARIS_SHORT))
					.set_flags(ALLEGRO_MENU_ITEM_DISABLED)
				)
			});
	}

	WindowControl::WindowControl(ConfigManager& cfg)
		: conf(cfg), dispev(disp), raw_menu(), menuev(raw_menu)
	{
		cout << IMGVW_STANDARD_START_WINDOWCTL << "Loading WindowManager...";

		current_texture = make_hybrid<texture>(); // replaceable
		current_file = make_hybrid<file>(); // replaceable

		texture_show_fixed.texture_insert(current_texture);
		texture_show_fixed.set<float>(enum_sprite_float_e::OUT_OF_SIGHT_POS, 0.0f);
		texture_show_fixed.set<float>(enum_sprite_float_e::SCALE_X, 2.0f);
		texture_show_fixed.set<float>(enum_sprite_float_e::SCALE_Y, 2.0f);

		const auto options = Lunaris::get_current_modes();
		for (const auto& it : options) { if (it.freq > found_max_fps_of_computer) found_max_fps_of_computer = it.freq; }
		if (found_max_fps_of_computer < min_fps) found_max_fps_of_computer = min_fps;

		disp.create(display_config()
			.set_display_mode(
				display_options()
				.set_width(conf.read_conf().get_as<int>("general", "width"))
				.set_height(conf.read_conf().get_as<int>("general", "height"))
			)
			.set_vsync(true)
			.set_window_title(get_app_name())
			.set_fullscreen(conf.read_conf().get_as<bool>("general", "was_fullscreen"))
			.set_extra_flags(ALLEGRO_OPENGL | ALLEGRO_RESIZABLE | (conf.read_conf().get_as<bool>("general", "was_fullscreen") ? ALLEGRO_FULLSCREEN_WINDOW : 0))
			.set_auto_economy_mode(false)
			.set_samples(8)
			.set_framerate_limit(found_max_fps_of_computer)
			.set_economy_framerate_limit(min_fps)
		);

		disp.post_task_on_destroy([this] { current_texture.reset_shared(); });
		
		disp.set_icon_from_icon_resource(IDI_ICON1);
		setup_menu();

		dispev.hook_event_handler([this](display_event& ev) { do_events(ev); });
		menuev.hook_event_handler([this](menu_event& ev) { do_menu(ev); });

		if (!conf.read_conf().get_as<bool>("general", "was_fullscreen")) show_menu();
		nv.load(conf.read_conf());

		if (auto lastt = conf.read_conf().get_as<unsigned long long>("general", "last_time_version_check"); static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()) - lastt > delta_time_seconds_next_check) {
			conf.write_conf().set("general", "last_time_version_check", static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
			throw_async([this] {
				nv.update();

				if (nv.got_data) {
					if (nv.has_update) { // apply has new version
						raw_menu.find_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE)).set_caption("New version: " + nv.tag_name);

						nv.store(conf.write_conf());
					}
					else {
						raw_menu.find_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE)).set_caption("Up to date");
					}
				}
				else {
					raw_menu.find_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE)).set_caption("Up to date");
				}
			});
		}
		else if (conf.read_conf().get_as<bool>("general", "had_update_last_time")) {
			const auto __tmp = conf.read_conf().get("general", "last_version_check");
			if (__tmp == versioning) {
				conf.write_conf().set("general", "had_update_last_time", false);
				nv.has_update = false;
				raw_menu.find_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE)).set_caption("Updated!");
			}
			else { // apply has new version
				raw_menu.find_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE)).set_caption("New version: " + __tmp);
			}
		}
		else {
			raw_menu.find_id(static_cast<uint16_t>(WindowControl::event_menu::CHECK_UPDATE)).set_caption("Up to date");
		}
	}

	WindowControl::~WindowControl()
	{
		if (__async_task.valid()) __async_task.get(); // wait for async thing
		conf.write_conf().flush();
		timed_bomb tb([] {std::terminate(); }, 5.0);
		destroy();
		tb.defuse();
	}

	void WindowControl::set_mode(const display_mode mod)
	{
		mode_change_time = al_get_time();
		curr_mode = mod;
	}

	std::future<bool> WindowControl::replace_image(hybrid_memory<file>&& fp, const std::string& newnam)
	{
		mode_change_time = al_get_time();
		curr_mode = display_mode::LOADING;
		bomb back_to_show([this] {
			mode_change_time = al_get_time();
			curr_mode = display_mode::SHOW;
		}); // guaranteed back to show lol

		auto fut = std::async(std::launch::async, [ff = std::move(fp), bmbb = std::move(back_to_show), this, newnam]() mutable {
			hybrid_memory<texture> test;

			bool isgif = is_gif(*ff);
			bool opened_cool = false;

			if (isgif) {
				test = make_hybrid_derived<texture, texture_gif>();
				opened_cool = ((texture_gif*)test.get())->load(ff);
			}
			else {
				test = make_hybrid<texture>();
				opened_cool = test->load(ff);
			}

			if (!opened_cool) {
				al_show_native_message_box(nullptr, "Failed loading image!", "Failed while trying to open file! Aborted!", "The latest file you tried to open isn't possible to open as is. Sorry about that.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
				return false;
			}

			if (isgif) {
				last_was_gif = true;
				const double interv = ((texture_gif*)test.get())->get_interval_shortest();
				const double fpsmin = interv > 0.0 ? 1.0 / interv : 0.0;
				if (fpsmin == 0.0) {
					disp.set_economy_fps(min_fps);
				}
				else {
					double resulting_fps = fpsmin;
					while (resulting_fps > 0.0 && resulting_fps < min_fps && resulting_fps < found_max_fps_of_computer) // if anim is like 7.457 fps, min_fps won't be good.
					{
						resulting_fps += fpsmin;
					}
					disp.set_economy_fps(resulting_fps);
				}
			}
			else {
				last_was_gif = false;
				disp.set_economy_fps(min_fps);
			}

			current_texture.replace_shared(test.reset_shared());
			current_file.replace_shared(ff.reset_shared());

			reset_camera();

			disp.set_window_title((get_app_name() + " - " + newnam).substr(0, 200));
			return true;
		});

		return fut;
	}

	void WindowControl::on_event(std::function<void(display_event&)> f)
	{
		events_handlr = f;
	}

	void WindowControl::on_menu(std::function<void(menu_event&)> f)
	{
		menu_handlr = f;
	}

	void WindowControl::set_camera_x(const float v)
	{
		texture_show_fixed.set<float>(enum_sprite_float_e::POS_X, v);
	}

	void WindowControl::set_camera_y(const float v)
	{
		texture_show_fixed.set<float>(enum_sprite_float_e::POS_Y, v);
	}

	void WindowControl::set_camera_z(const float v)
	{
		camera_zoom = v;
		update_camera = true;
	}

	void WindowControl::set_camera_r(const float v)
	{
		camera_rot = v;
		update_camera = true;
	}

	float WindowControl::get_camera_x() const
	{
		return texture_show_fixed.get<float>(enum_sprite_float_e::POS_X);
	}

	float WindowControl::get_camera_y() const
	{
		return texture_show_fixed.get<float>(enum_sprite_float_e::POS_Y);
	}

	float WindowControl::get_camera_z() const
	{
		return camera_zoom;
	}

	float WindowControl::get_camera_r() const
	{
		return camera_rot;
	}

	future<bool> WindowControl::show_menu()
	{
		return disp.post_task([this] { raw_menu.show(disp); return true; });
	}

	future<bool> WindowControl::hide_menu()
	{
		return disp.post_task([this] { raw_menu.hide(disp); return true; });
	}

	const display& WindowControl::get_raw_display() const
	{
		return disp;
	}

	bool WindowControl::is_fullscreen() const
	{
		return disp.get_flags() & ALLEGRO_FULLSCREEN_WINDOW;
	}

	void WindowControl::set_quit()
	{
		throw_async([this] { destroy(); });
	}

	void WindowControl::set_update_camera()
	{
		update_camera = true;
	}

	void WindowControl::flip()
	{
		do_draw();
		disp.flip();
	}

	void WindowControl::yield()
	{
		while (!disp.empty()) flip();
	}

	void WindowControl::wakeup_draw()
	{
		last_update_ext = al_get_time();
	}

	display* WindowControl::operator->()
	{
		return &disp;
	}

	const display* WindowControl::operator->() const
	{
		return &disp;
	}

	bool is_gif(file& fp)
	{
		if (!fp.is_open()) return false;
		fp.seek(0, file::seek_mode_e::BEGIN);

		std::string comp(__gif_default_start.size(), '\0');
		fp.read(comp.data(), __gif_default_start.size());
		fp.seek(0, file::seek_mode_e::BEGIN);
		
		return comp == __gif_default_start;
	}

}