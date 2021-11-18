#pragma once

#include <Lunaris/all.h>
#include <nlohmann/json.hpp>

#include <future>
#include <thread>
#include <shellapi.h>

#include "configuration.h"
#include "shared.h"
#include "resource.h"

using namespace Lunaris;

#define IMGVW_STANDARD_START_WINDOWCTL Lunaris::console::color::DARK_PURPLE << "[WINDOW] " << Lunaris::console::color::GRAY 

namespace ImageViewer {

	const std::string __gif_default_start = "GIF89a";
	const double max_fps = 240.0;
	const double loading_fps = 60.0;
	const double min_fps = 20.0;
	const double deep_sleep_fps = 2.0;
	const float animation_transparency_load = 0.008f;
	const double time_sleep = 0.35;
	const double deep_sleep_time = 10.0;

	std::vector<menu_each_menu> make_menu();

	class WindowControl {
	public:
		enum class display_mode { LOADING, SHOW };
		enum class event_menu {
			FILE,
				FILE_OPEN,
				FILE_FOLDER_OPEN,
				FILE_PASTE_URL,
				FILE_EXIT,
			CHECK_UPDATE,
				UPDATE_VERSION,
				CURRENT_VERSION,
				ABOUT_LUNARIS_SHORT,
				RELEASE_DATE
		};
	private:
		display disp;
		display_event_handler dispev;

		menu raw_menu;
		menu_event_handler menuev;

		ConfigManager& conf;

		struct new_version_data {
		public:
			std::string tag_name;
			bool got_data = false;
			bool has_update = false;

			void update();
			void load(const config&);
			void store(config&);
		} nv;

		display_mode curr_mode = display_mode::LOADING;
		double mode_change_time = 0.0;
		double last_update_ext = 0.0;
		bool last_was_gif = false; // disables deep sleep
		double found_max_fps_of_computer = 0.0;

		hybrid_memory<texture> current_texture;
		hybrid_memory<file> current_file;
		std::future<void> __async_task;

		block texture_show_fixed;

		bool no_bar = false;
		std::atomic_bool update_camera = true;
		float camera_zoom = 1.0f;
		float camera_rot = 0.0f;

		safe_data<std::function<void(display_event&)>> events_handlr;
		safe_data<std::function<void(menu_event&)>> menu_handlr;

		void throw_async(const std::function<void(void)>);

		void update_fps_limiters();

		void do_draw();
		void do_events(display_event&);
		void do_menu(menu_event&);

		void reset_camera();

		void destroy();
	public:
		WindowControl(ConfigManager&);
		~WindowControl();

		void set_mode(const display_mode);
		// file, name
		std::future<bool> replace_image(hybrid_memory<file>&&, const std::string&);

		void on_event(std::function<void(display_event&)>);
		void on_menu(std::function<void(menu_event&)>);

		// posx
		void set_camera_x(const float);
		// posy
		void set_camera_y(const float);
		// zoom
		void set_camera_z(const float);
		// rotation
		void set_camera_r(const float);

		float get_camera_x() const;
		float get_camera_y() const;
		float get_camera_z() const;
		float get_camera_r() const;

		future<bool> show_menu();
		future<bool> hide_menu();

		const display& get_raw_display() const;

		bool is_fullscreen() const;

		// like closing the display:
		void set_quit();

		void set_update_camera();

		void flip();
		void yield();

		void wakeup_draw();

		display* operator->();
		const display* operator->() const;
	};

	// this will rewind the file lol
	bool is_gif(file&);

}