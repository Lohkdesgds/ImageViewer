#include "../include/window.h"
#include "../include/configurer.h"
#include "../icon/app.h"

Window::camera& Window::camera::operator<<(const camera& o)
{
	if (m_last_upd_cam > al_get_time()) {
		m_transf.use();
		return *this;
	}
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
	if (!al_is_system_installed()) al_init();

	auto& gconf = g_conf();

	int suggested_ss[2]{ 0,0 };
	if (auto c = gconf.get("general", "screen_width"); !c.empty()) suggested_ss[0] = std::atoi(c.c_str());
	if (auto c = gconf.get("general", "screen_height"); !c.empty()) suggested_ss[1] = std::atoi(c.c_str());

	auto di = _get_target_display_info();

	if (suggested_ss[0] >= prop_minimum_screen_size[0]) {
		di.size[0] = suggested_ss[0];
	}
	else {
		di.size[0] *= prop_scale_of_screen;
	}

	if (suggested_ss[1] >= prop_minimum_screen_size[1]) {
		di.size[1] = suggested_ss[1];
	}
	else {
		di.size[1] *= prop_scale_of_screen;
	}


	auto mpos = AllegroCPP::Mouse_cursor::get_pos();

	mpos.first -= di.size[0] * 0.5f;
	mpos.second -= di.size[1] * 0.5f;

	// safe pos
	if (mpos.second < di.min_h) mpos.second = di.min_h;

	al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
	al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);

	m_disp = std::shared_ptr<AllegroCPP::Display>(new AllegroCPP::Display({ di.size[0], di.size[1] }, prop_base_window_name, ALLEGRO_RESIZABLE, mpos));
	m_disp->set_icon_from_resource(IDI_ICON1);
	m_font = std::make_unique<AllegroCPP::Font>();

	m_evqu << m_timer;
	m_evqu << m_evcustom;

	m_timer.start();
	return true;
}

bool Window::_loop()
{
	if (!m_disp) return false;
	if (!m_mutex.run()) return true;

	if (m_task) {
		m_task();
		m_task = {};
	}

	m_evqu.wait_for_event(0.2f); // it is timer for sure.
	const double timer_speed_now = m_timer.get_speed();

	//m_disp->clear_to_color(al_map_rgb(127 + 128 * cos(al_get_time()), 25, m_curr_cam.m_need_refresh ? 255 : 25));
	m_disp->clear_to_color(al_map_rgb(25, 25, 25));

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
		al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_VIDEO_BITMAP);
		al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY);
		al_convert_bitmaps();
	}

	if (m_bmp) {

		if (!m_curr_cam.m_need_refresh) {
			if (timer_speed_now != prop_timer_min_refresh_rate) m_timer.set_speed(prop_timer_min_refresh_rate);
		}
		else if (timer_speed_now != prop_timer_max_refresh_rate) m_timer.set_speed(prop_timer_max_refresh_rate);

		m_bmp.draw(0, 0,
			{
				{ AllegroCPP::bitmap_rotate_transform{ m_bmp.get_width() * 0.5f, m_bmp.get_height() * 0.5f, 0.0f } },
				{ AllegroCPP::bitmap_scale{ currscal, currscal }}
			});
	}
	else if (m_bmp_alt) {
		const double gif_fps = (m_bmp_alt.get_interval_shortest() > 0.0) ? (1.0 / m_bmp_alt.get_interval_shortest()) : prop_timer_min_refresh_rate;

		if (!m_curr_cam.m_need_refresh) {
			if (timer_speed_now != gif_fps) {
				m_timer.set_speed(1.0 / gif_fps);
			}
		}
		else if (timer_speed_now != prop_timer_max_refresh_rate) m_timer.set_speed(prop_timer_max_refresh_rate);

		m_bmp_alt.draw(0, 0,
			{
				{ AllegroCPP::bitmap_rotate_transform{ m_bmp_alt.get_width() * 0.5f, m_bmp_alt.get_height() * 0.5f, 0.0f } },
				{ AllegroCPP::bitmap_scale{ currscal, currscal }}
			});
	}


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
	auto& gconf = g_conf();

	if (m_was_fullscreen) { // is fullscreen
		gconf.set("general", "screen_width", std::to_string(m_last_display_before_fullscreen.size[0]));
		gconf.set("general", "screen_height", std::to_string(m_last_display_before_fullscreen.size[1]));
	}
	else {
		gconf.set("general", "screen_width", std::to_string(m_disp->get_width()));
		gconf.set("general", "screen_height", std::to_string(m_disp->get_height()));
	}

	g_save_conf();

	m_disp.reset();
	m_evqu.remove_source(m_timer);
	m_timer.stop();
}

bool Window::_manage()
{
	switch (m_stage) {
	case 0:
		++m_stage;
		return _build();
	case 1:
		return _loop();
	default:
		_destroy();
		m_stage = 0;
		return false;
	}
}

Window::display_info Window::_get_target_display_info()
{
	Window::display_info di;

	auto mpos = AllegroCPP::Mouse_cursor::get_pos();

	for (int p = 0; p < al_get_num_video_adapters(); ++p)
	{
		AllegroCPP::Monitor_info mi(p);

		if (
			mpos.first >= mi.m_info.x1 && mpos.first < mi.m_info.x2 &&
			mpos.second >= mi.m_info.y1 && mpos.second < mi.m_info.y2
			) {
			di.size[0] = mi.get_width();
			di.size[1] = mi.get_height();
			di.offset[0] = mi.m_info.x1;
			di.offset[1] = mi.m_info.y1;
			di.min_h = mi.m_info.y1 + 20;
			break;
		}
	}

	return di;
}

Window::Window()
{
	m_thr.create([this] {return _manage(); }, AllegroCPP::Thread::Mode::NORMAL);
}

Window::~Window()
{
	Lunaris::fast_lock_guard l(m_mutex); // force stop
	m_stage = 2; // end

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
	if (m_disp) {
		m_disp->acknowledge_resize();
		m_disp->resize({ m_disp->get_width(), m_disp->get_height() }); // size 0 bug, workaround
		m_disp->acknowledge_resize();

		if (m_disp->get_width() < prop_minimum_screen_size[0] || m_disp->get_height() < prop_minimum_screen_size[1]) {
			m_disp->resize({
				m_disp->get_width() < prop_minimum_screen_size[0] ? prop_minimum_screen_size[0] : m_disp->get_width(),
				m_disp->get_height() < prop_minimum_screen_size[1] ? prop_minimum_screen_size[1] : m_disp->get_height()
			});
			m_disp->acknowledge_resize();
		}
	}
}

Window::camera& Window::cam()
{
	return m_targ_cam;
}

void Window::post_update()
{
	m_evcustom.emit((void*)1, [](auto) {}, prop_self_event_id, [] {});
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
	if (m_disp) m_disp->set_title(prop_base_window_name + s);
}

void Window::post_wait(std::function<void(void)> f)
{
	if (!f) return;
	{
		Lunaris::fast_lock_guard l(m_mutex);
		m_task = f;
	}
	while (m_task) al_rest(1.0 / 200);
}

void Window::toggle_fullscreen()
{
	if (!m_disp) return;
	Lunaris::fast_lock_guard l(m_mutex);

	if (m_was_fullscreen = !m_was_fullscreen) { // should be fullscreen
		auto& i = m_last_display_before_fullscreen;

		i.size[0] = m_disp->get_width();
		i.size[1] = m_disp->get_height();

		const auto p = m_disp->get_position();

		i.offset[0] = p.first;
		i.offset[1] = p.second;

		const auto targ = _get_target_display_info();

		m_disp->set_flag(ALLEGRO_NOFRAME, true);
		m_disp->set_position({ targ.offset[0], targ.offset[1] });
		m_disp->resize({ targ.size[0], targ.size[1] });
		m_disp->acknowledge_resize();
		m_disp->set_icon_from_resource(IDI_ICON1);
	}
	else { // undo fullscreen
		const auto& targ = m_last_display_before_fullscreen;

		m_disp->resize({ targ.size[0], targ.size[1] });
		m_disp->acknowledge_resize();
		m_disp->set_position({ targ.offset[0], targ.offset[1] });
		m_disp->set_flag(ALLEGRO_NOFRAME, false);
		m_disp->set_icon_from_resource(IDI_ICON1);
	}
}

std::string Window::clipboard_text() const
{
	return m_disp ? m_disp->get_clipboard_text() : "";
}


Window::operator ALLEGRO_EVENT_SOURCE* () const
{
	return m_disp ? m_disp->get_event_source() : nullptr;
}
