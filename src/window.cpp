#include "../include/window.h"
#include "../icon/app.h"

Window::camera& Window::camera::operator<<(const camera& o)
{
	if (m_last_upd_cam > al_get_time()) return *this;
	m_last_upd_cam = al_get_time() + prop_update_delta;

	const float plus = (prop_smoothness_motion + 1.0f);

	const float n_m_posx = (prop_smoothness_motion * m_posx + o.m_posx) / plus;
	const float n_m_posy = (prop_smoothness_motion * m_posy + o.m_posy) / plus;
	const float n_m_rot = (prop_smoothness_motion * m_rot + o.m_rot) / plus;
	const float n_m_scale = (prop_smoothness_motion * m_scale + o.m_scale) / plus;

	m_need_refresh = (fabs(m_posx - n_m_posx) * 0.01f * m_scale + fabs(m_posy - n_m_posy) * 0.01f * m_scale + fabs(m_rot - n_m_rot) * 10.0f + fabs(m_scale - n_m_scale) * 10.0f) > prop_min_factor_update;

	m_posx = n_m_posx;
	m_posy = n_m_posy;
	m_rot = n_m_rot;
	m_scale = n_m_scale;

	const int cdx = al_get_display_width(al_get_current_display());
	const int cdy = al_get_display_height(al_get_current_display());


	m_transf.identity();
	m_transf.scale({ m_scale, m_scale });
	m_transf.translate({ m_posx * (m_scale > 1.0f ? m_scale : 1.0f), m_posy * (m_scale > 1.0f ? m_scale : 1.0f) });
	m_transf.rotate(m_rot);
	m_transf.translate({ cdx * 0.5f, cdy * 0.5f });
	m_transf.use();

	return *this;
}

void Window::camera::reset()
{
	m_posx = 0;
	m_posy = 0;
	m_rot = 0;
	m_scale = 1.0f;
}

void Window::camera::translate(int x, int y)
{
	const float sx = (x * prop_mouse_transl_factor) * (m_scale > 1.0f ? 1.0f / m_scale : 1.0f);
	const float sy = (y * prop_mouse_transl_factor) * (m_scale > 1.0f ? 1.0f / m_scale : 1.0f);

	m_posx += sx * cos(m_rot) + sy * sin(m_rot);
	m_posy += sy * cos(m_rot) - sx * sin(m_rot);
}

void Window::camera::scale(int w)
{
	m_scale += (w * prop_mouse_scale_factor) * m_scale;
	if (m_scale <= static_cast<float>(1e-2)) m_scale = static_cast<float>(1e-2);
}

void Window::camera::rotate(int r)
{
	m_rot += (r * prop_mouse_rot_factor);
}


bool Window::_build()
{
	int dw = 1280, dh = 720;
	int min_h = 20;

	if (!al_is_system_installed()) al_init();
	auto mpos = AllegroCPP::Mouse_cursor::get_pos();
	
	for (int p = 0; p < al_get_num_video_adapters(); ++p)
	{
		AllegroCPP::Monitor_info mi(p);

		if (
			mpos.first >= mi.m_info.x1 && mpos.first < mi.m_info.x2 &&
			mpos.second >= mi.m_info.y1 && mpos.second < mi.m_info.y2
		) {
			dw = mi.get_width() * prop_scale_of_screen;
			dh = mi.get_height() * prop_scale_of_screen;
			min_h = mi.m_info.y1 + 20;
			break;
		}
	}

	mpos.first -= dw * 0.5f;
	mpos.second -= dh * 0.5f;

	// safe pos
	if (mpos.second < min_h) mpos.second = min_h;

	m_disp = std::shared_ptr<AllegroCPP::Display>(new AllegroCPP::Display({ dw, dh }, prop_base_window_name, ALLEGRO_RESIZABLE, mpos));
	m_disp->set_icon_from_resource(IDI_ICON1);
	m_font = std::make_unique<AllegroCPP::Font>();
	return true;
}

bool Window::_loop()
{
	if (!m_disp) return false;
	if (!m_mutex.run()) return true;

	m_disp->clear_to_color(al_map_rgb(127 + 128 * cos(al_get_time()), 25, m_curr_cam.m_need_refresh ? 255 : 25));

	float currscal = 1.0f;

	if (m_bmp) {
		const float dx = m_disp->get_width() * 1.0f / m_bmp.get_width();
		const float dy = m_disp->get_height() * 1.0f / m_bmp.get_height();

		currscal = dx < dy ? dx : dy;
	}
	else if (m_bmp_alt) {
		const float dx = m_disp->get_width() * 1.0f / m_bmp_alt.get_width();
		const float dy = m_disp->get_height() * 1.0f / m_bmp_alt.get_height();

		currscal = dx < dy ? dx : dy;
	}

	// smooth insert
	m_curr_cam << m_targ_cam;

	if (m_convert_bmps) {
		m_convert_bmps = false;
		al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR | ALLEGRO_VIDEO_BITMAP);
		al_convert_bitmaps();
	}

	if (m_bmp) {

		if (!m_curr_cam.m_need_refresh) al_rest(1.0 / 15);

		m_bmp.draw(0, 0,
			{
				{ AllegroCPP::bitmap_rotate_transform{ m_bmp.get_width() * 0.5f, m_bmp.get_height() * 0.5f, 0.0f } },
				{ AllegroCPP::bitmap_scale{ currscal, currscal }}
			});
	}
	else if (m_bmp_alt) {
		m_bmp_alt.draw(0, 0,
			{
				{ AllegroCPP::bitmap_rotate_transform{ m_bmp.get_width() * 0.5f, m_bmp.get_height() * 0.5f, 0.0f } },
				{ AllegroCPP::bitmap_scale{ currscal, currscal }}
			});
	}
		//m_curr_cam.m_posx,// + m_disp->get_width() * 0.5f,
		//m_curr_cam.m_posy, {// + m_disp->get_height() * 0.5f, {
		//AllegroCPP::bitmap_rotate_transform{ m_bmp.get_width() * 0.5f, m_bmp.get_height() * 0.5f, m_curr_cam.m_rot }
		////AllegroCPP::bitmap_scale{ m_curr_cam.m_scale* currscal, m_curr_cam.m_scale* currscal }
	//});


	if (m_overlay_center.length()) {
		if (al_get_time() > m_time_to_erase) {
			m_overlay_center.clear();
		}
		else {
			double factor = m_time_to_erase - al_get_time();
			if (factor > 2.0) factor = 2.0;
			if (factor < 0.0) factor = 0.0;
			factor /= 2.0;

			AllegroCPP::Transform t;
			t.scale({ 2.0f, 2.0f });
			t.use();
			m_font->draw({ m_disp->get_width() * 0.25f + 2, 6.0f }, m_overlay_center, al_map_rgba_f(0.0f, 0.0f, 0.0f, static_cast<float>(factor)), AllegroCPP::text_alignment::CENTER);
			m_font->draw({ m_disp->get_width() * 0.25f, 4.0f }, m_overlay_center, al_map_rgba_f(1.0f, 1.0f, 1.0f, static_cast<float>(factor)), AllegroCPP::text_alignment::CENTER);
		}
	}

	m_disp->flip();

	return true;
}

void Window::_destroy()
{
	m_disp.reset();
}

bool Window::_manage()
{
	switch (stage) {
	case 0:
		++stage;
		return _build();
	case 1:
		return _loop();
	default:
		_destroy();
		stage = 0;
		return false;
	}
}

Window::Window()
{
	m_thr.create([this] {return _manage(); }, AllegroCPP::Thread::Mode::NORMAL);
}

Window::~Window()
{
	Lunaris::fast_lock_guard l(m_mutex); // force stop
	stage = 2; // end

	m_thr.join();
}

void Window::put(AllegroCPP::Bitmap&& b, const bool reset_cam)
{
	Lunaris::fast_lock_guard l(m_mutex);
	m_bmp = std::move(b);
	m_bmp_alt.destroy();
	m_convert_bmps = true;
	if (reset_cam) cam().reset();
}

void Window::put(AllegroCPP::GIF&& b, const bool reset_cam)
{
	Lunaris::fast_lock_guard l(m_mutex);
	m_bmp.destroy();
	m_bmp_alt = std::move(b);
	m_convert_bmps = true;
	if (reset_cam) cam().reset();
}

void Window::ack_resize()
{
	if (m_disp) m_disp->acknowledge_resize();
}

Window::camera& Window::cam()
{
	return m_targ_cam;
}

bool Window::has_display() const
{
	return m_disp.get();
}

void Window::set_text(const std::string& s, double for_sec)
{
	Lunaris::fast_lock_guard l(m_mutex);
	m_overlay_center = s;
	m_time_to_erase = for_sec + al_get_time();
}

void Window::set_title(const std::string& s)
{
	m_disp->set_title(prop_base_window_name + s);
}


Window::operator ALLEGRO_EVENT_SOURCE* () const
{
	return m_disp ? m_disp->get_event_source() : nullptr;
}
