#pragma once

#include <Graphics.h>
#include <System.h>
#include <Mutex/mutex.h>

#include <memory>

constexpr float prop_scale_of_screen = 0.6f;
constexpr float prop_smoothness_motion = 4.5f;
constexpr float prop_mouse_transl_factor = 1.25f;
constexpr float prop_mouse_scale_factor = 0.2f;
constexpr float prop_mouse_rot_factor = ALLEGRO_PI * 0.0625f;
constexpr double prop_update_delta = 1.0f / 60;
constexpr double prop_min_factor_update = 1.0f / 500.0f;
constexpr double prop_timer_max_refresh_rate = 1.0 / 240;
constexpr double prop_timer_min_refresh_rate = 1.0 / 4;
constexpr int prop_self_event_id = 1024;
constexpr int prop_minimum_screen_size[2] = {400, 240};

const std::string prop_base_window_name = "ImageViewer V5";

class Window {
	struct camera {
		AllegroCPP::Transform m_transf;
		float m_posx{}, m_posy{}, m_rot{}, m_scale = 1.0f;
		double m_last_upd_cam = 0.0;
		bool m_need_refresh = true;

		camera& operator<<(const camera&);

		void reset();
		void translate(int, int);
		void scale(int);
		void rotate(int);
	};
	struct display_info {
		int offset[2] = { 0, 0 };
		int min_h = 20;
		int size[2] = { 1280, 720 };
	};

	std::shared_ptr<AllegroCPP::Display> m_disp;
	
	AllegroCPP::Bitmap m_bmp;
	AllegroCPP::GIF m_bmp_alt;
	AllegroCPP::Timer m_timer{prop_timer_max_refresh_rate};
	AllegroCPP::Event_queue m_evqu;
	AllegroCPP::Event_custom m_evcustom;
	std::unique_ptr<AllegroCPP::Font> m_font;

	bool m_convert_bmps = false;
	
	std::string m_overlay_center;
	double m_time_to_erase = 0;

	camera m_targ_cam, m_curr_cam;
	//camera* m_used = nullptr;


	AllegroCPP::Thread m_thr;
	Lunaris::fast_one_way_mutex m_mutex;
	std::function<void(void)> m_task;
	uint8_t m_stage = 0;

	display_info m_last_display_before_fullscreen{};
	bool m_was_fullscreen = false;

	bool _build();
	bool _loop();
	void _destroy();

	bool _manage();

	display_info _get_target_display_info();
public:
	Window();
	~Window();

	void put(AllegroCPP::Bitmap&&, const bool = true);
	void put(AllegroCPP::GIF&&, const bool = true);
	void ack_resize();

	camera& cam();
	void post_update();
	bool has_display() const;

	void set_text(const std::string&, double for_sec = 5.0);
	void set_title(const std::string&);
	void post_wait(std::function<void(void)>);

	void toggle_fullscreen();
	std::string clipboard_text() const;

	operator ALLEGRO_EVENT_SOURCE*() const;
};