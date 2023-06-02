#pragma once

#include <Graphics.h>
#include <System.h>
#include <Mutex/mutex.h>

#include <memory>

constexpr float prop_scale_of_screen = 0.6f;
constexpr float prop_smoothness_motion = 9.0f;
constexpr float prop_mouse_transl_factor = 1.25f;
constexpr float prop_mouse_scale_factor = 0.2f;
constexpr float prop_mouse_rot_factor = 0.1f;
constexpr double prop_update_delta = 1.0f / 120;
constexpr double prop_min_factor_update = 1.0f / 500.0f;

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

	std::unique_ptr<AllegroCPP::Display> m_disp;
	AllegroCPP::Bitmap m_bmp;
	bool m_convert_bmps = false;
	camera m_targ_cam, m_curr_cam;


	AllegroCPP::Thread m_thr;
	Lunaris::fast_one_way_mutex m_mutex;
	uint8_t stage = 0;

	bool _build();
	bool _loop();
	void _destroy();

	bool _manage();
public:
	Window();
	~Window();

	void put(AllegroCPP::Bitmap&&, const bool = true);
	void ack_resize();

	camera& cam();
	bool has_display() const;

	operator ALLEGRO_EVENT_SOURCE*() const;
};