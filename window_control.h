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
	const double max_fps = 250.0;
	const double min_fps = 25.0;
	const float animation_transparency_load = 0.005f;

	std::vector<menu_each_menu> make_menu();
	//std::vector<menu_each_menu> make_rmenu();

	class WindowControl {
	public:
		enum class display_mode { LOADING, SHOW };
		enum class event_menu {
			FILE,
				FILE_OPEN,
				FILE_FOLDER_OPEN,
				FILE_PASTE_URL,
				FILE_EXIT,
			//TOGGLE_BAR_MODEL_MENU,
			//	TOGGLE_BAR_MODEL,
			CHECK_UPDATE,
				UPDATE_VERSION,
				CURRENT_VERSION,
				ABOUT_LUNARIS_SHORT,
				RELEASE_DATE
		};
	private:
		display_async disp;
		display_event_handler dispev;

		menu raw_menu;
		menu_event_handler menuev;
		//menu rclk;
		//menu_event_handler menuevrc;


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

		hybrid_memory<texture> current_texture;
		hybrid_memory<file> current_file;
		std::future<void> __async_task;

		block texture_show_fixed;

		bool no_bar = false;
		std::atomic_bool update_camera = true;
		float camera_zoom = 1.0f;
		float camera_rot = 0.0f;
		double min_fps_on_economy = max_fps;

		safe_data<std::function<void(void)>> quick_quit;
		safe_data<std::function<void(display_event&)>> events_handlr;
		safe_data<std::function<void(menu_event&)>> menu_handlr;

		void throw_async(const std::function<void(void)>);

		void do_draw();
		void do_events(display_event&);
		void do_menu(menu_event&);

		void reset_camera();

		void destroy();
	public:
		WindowControl(ConfigManager&);
		~WindowControl();

		void set_mode(const display_mode);
		// file, is gif?
		std::future<bool> replace_image(hybrid_memory<file>&&, const std::string&);

		void on_quit(std::function<void(void)>);
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

		//future<bool> right_click();

		const display_async& get_raw_display() const;

		bool is_fullscreen() const;

		// like closing the display:
		void set_quit();

		void set_update_camera();

		display_async* operator->();
		const display_async* operator->() const;
	};

	// this will rewind the file lol
	bool is_gif(file&);

}