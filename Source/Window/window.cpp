#include "window.h"

using namespace Lunaris;


void Window::on_disp(const display_event& ev)
{
	switch (ev.get_type()) {
	case ALLEGRO_EVENT_DISPLAY_CLOSE:
		disp.destroy();
		break;
	}
}

void Window::on_mouse(const int id, const mouse::mouse_event& ev)
{
	ALLEGRO_KEYBOARD_STATE keybst;
	al_get_keyboard_state(&keybst);

	const auto is_key_down = [&](const int kd) { return al_key_down(&keybst, kd); };
		
	if (ev.is_button_pressed(0))
	{
		if (frames.size()) {
			bool updated = false;
			frames.safe([this, &ev, &updated](std::vector<Frame>& v) {
				std::vector<Frame>::iterator it = v.end();
				
				for (it = v.begin(); it != v.end(); ++it) {
					if (it->hit(ev.real_posx, ev.real_posy, disp)) {
						if (it != v.begin()) {
							std::iter_swap(it, v.begin());
							updated = true;
						}
						break;
					}
				}
				if (it == v.end()) return;

				it->posx += ev.raw_mouse_event.dx;
				it->posy += ev.raw_mouse_event.dy;
			});

			if (updated)
				f_frames_update.csafe([&](const std::function<void(const safe_vector<Frame>&)>& f) { if (f) f(frames); });
		}
	}
	else if (ev.raw_mouse_event.button == 2 && ev.raw_mouse_event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
	{
		frames.safe([this, &ev](std::vector<Frame>& v) {
			std::vector<Frame>::iterator it = v.end();
			size_t p = 0;

			for (it = v.begin(); it != v.end(); ++it) {
				if (it->hit(ev.real_posx, ev.real_posy, disp)) {
					f_rclick.csafe([&](const std::function<void(const size_t, Frame&)>& f) { if (f) f(p, *it); });
					break;
				}
				++p;
			}
			if (it == v.end()) return;

			it->posx += ev.raw_mouse_event.dx;
			it->posy += ev.raw_mouse_event.dy;
		});

	}
	else if (ev.got_scroll_event())
	{
		const int ds = ev.scroll_event_id(1);
		if (ds == 0) return;
		
		if (frames.size()) {
			bool updated = false;
			frames.safe([this, &ev, &ds, &updated](std::vector<Frame>& v) {
				std::vector<Frame>::iterator it = v.end();
				
				for (it = v.begin(); it != v.end(); ++it) {
					if (it->hit(ev.real_posx, ev.real_posy, disp)) {
						if (it != v.begin()) {
							std::iter_swap(it, v.begin());
							updated = true;
						}
						break;
					}
				}
				if (it == v.end()) return;

				it->scale *= (ds > 0 ? (1.0f / 0.97f) : (0.97f));
			});

			if (updated)
				f_frames_update.csafe([&](const std::function<void(const safe_vector<Frame>&)>& f) { if (f) f(frames); });
		}
	}

	// do smth
}

Window::Window()
	: dispev(disp), mous(disp)
{
	const auto opts = Lunaris::get_current_modes();
	ALLEGRO_MONITOR_INFO moninf;
	al_get_monitor_info(0, &moninf);
	const int dx = moninf.x1 < moninf.x2 ? (moninf.x2 - moninf.x1) : (moninf.x1 - moninf.x2);
	const int dy = moninf.y1 < moninf.y2 ? (moninf.y2 - moninf.y1) : (moninf.y1 - moninf.y2);

	int max_fps = 60;
	for (const auto& each : opts) { if (each.width == dx && each.height == dy && each.freq > max_fps) max_fps = each.freq; }

	disp.create(display_config()
		.set_display_mode(display_options().set_width(600).set_height(600))
		.set_extra_flags(ALLEGRO_OPENGL|ALLEGRO_RESIZABLE)
		.set_fullscreen(false)
		.set_wait_for_display_draw(true)
		.set_vsync(true)
		.set_window_title(get_app_name())
		.set_framerate_limit(max_fps)
		.set_auto_economy_mode(false)
	);

	disp.set_icon_from_icon_resource(IDI_ICON1);

	al_install_keyboard(); // external

	dispev.hook_event_handler([this](const display_event& ev) { on_disp(ev); });
	mous.hook_event([this](const int id, const mouse::mouse_event& ev) { on_mouse(id, ev); });

	disp.post_task_on_destroy([this] {
		frames.safe([&](std::vector<Frame>& v) {
			for (auto& it : v) it.reset();
		});
		frames.clear();
	});
}

bool Window::draw()
{
	if (disp.empty()) return false;
	disp.set_as_target();

	color(16, 16, 16).clear_to_this();

	frames.csafe([this](const std::vector<Frame>& v) {
		for (auto it = v.rbegin(); it != v.rend(); ++it) {
			it->draw();
		}
	});

	disp.flip();

	return true;
}

future<bool> Window::push(const std::string& path)
{
	int fptyp = 0; // 0 == default, 1 == gif, 2 == URL

	if (std::regex_search(path, regex_link, std::regex_constants::match_any)) {
		fptyp = 2;
	}
	else {
		file tst;
		if (!tst.open(path, file::open_mode_e::READ_TRY)) return make_empty_future<bool>(false);
		std::string comp(__gif_default_start.size(), '\0');
		tst.read(comp.data(), __gif_default_start.size());
		tst.close();
		
		fptyp = (__gif_default_start == comp) ? 1 : 0;
	}

	switch (fptyp) {
	case 1: // GIF
	{
		cout << "Loading '" << path << "' as GIF";

		return disp.post_task([path, this]() -> bool {
			bool _gud = false;
			const std::string nnam = path.size() > max_item_name_size ? ("..." + path.substr(path.size() - max_item_name_size)) : path;

			frames.safe([&](std::vector<Frame>& v) {
				_gud = v.emplace_back(Frame{}).load([&]()->hybrid_memory<texture> {
					auto tx = make_hybrid_derived<texture, texture_gif>();
					if (!((texture_gif*)tx.get())->load(path)) {
						cout << "Can't load '" << path << "' as GIF";
						return {};
					}
					cout << "Loaded '" << path << "' as GIF";
					return tx;
				}, nnam);
			});

			if (_gud) f_frames_update.csafe([&](const std::function<void(const safe_vector<Frame>&)>& f) { if (f) f(frames); });

			return _gud;
		});
	}
	case 2: // URL
	{
		cout << "Loading '" << path << "' as URL";

		hybrid_memory<file> fp = make_hybrid_derived<file, tempfile>();
		downloader down;
		if (!down.get(path)) {
			cout << "Failed downloading '" << path << "' as URL";
			return make_empty_future(false);
		}
		
		std::string formt = "jpg"; // assume jpg, what could go wrong?
		bool is_gif = false;

		if (down.read().size() >= png_format.size() && strncmp(down.read().data(), png_format.data(), png_format.size()) == 0) {
			formt = "png";
		}
		else if (down.read().size() >= __gif_default_start.size() && strncmp(down.read().data(), __gif_default_start.data(), __gif_default_start.size()) == 0) {
			is_gif = true;
			formt = "gif";
		}

		if (!((tempfile*)fp.get())->open("IMGVW_XXXXXXX." + formt)) return make_empty_future(false);

		((tempfile*)fp.get())->write(down.read().data(), down.read().size());
		((tempfile*)fp.get())->flush();

		cout << "Working '" << path << "'...";

		return disp.post_task([is_gif, path, this, ffp = std::move(fp)]() -> bool {
			bool _gud = false;
			const std::string nnam = path.size() > max_item_name_size ? ("..." + path.substr(path.size() - max_item_name_size)) : path;

			frames.safe([&](std::vector<Frame>& v) {
				_gud = v.emplace_back(Frame{}).load([&]()->hybrid_memory<texture> {
					if (is_gif) {
						auto tx = make_hybrid_derived<texture, texture_gif>();
						if (!((texture_gif*)tx.get())->load(ffp)) {
							cout << "Can't load '" << path << "' as URL GIF";
							return {};
						}
						cout << "Loaded '" << path << "' as URL GIF";
						return tx;
					}
					else {
						auto tx = make_hybrid<texture>();
						if (!tx->load(ffp)) {
							cout << "Can't load '" << path << "' as URL";
							return {};
						}
						cout << "Loaded '" << path << "' as URL";
						return tx;
					}
				}, nnam);
			});

			if (_gud) f_frames_update.csafe([&](const std::function<void(const safe_vector<Frame>&)>& f) { if (f) f(frames); });

			return _gud;
		});

	}
	default: // FILE
	{
		cout << "Loading '" << path << "' as FILE";

		return disp.post_task([path, this]() -> bool {
			bool _gud = false;
			const std::string nnam = path.size() > max_item_name_size ? ("..." + path.substr(path.size() - max_item_name_size)) : path;

			frames.safe([&](std::vector<Frame>& v) {
				_gud = v.emplace_back(Frame{}).load([&]()->hybrid_memory<texture> {
					auto tx = make_hybrid<texture>();
					if (!tx->load(path)) {
						cout << "Can't load '" << path << "' as FILE";
						return {};
					}
					cout << "Loaded '" << path << "' as FILE";
					return tx;
				}, nnam);
			});

			if (_gud) f_frames_update.csafe([&](const std::function<void(const safe_vector<Frame>&)>& f) { if (f) f(frames); });

			return _gud;
		});
	}
	}
}

void Window::safe(std::function<void(std::vector<Frame>&)> f)
{
	frames.safe(f);
}

void Window::on_frames_update(std::function<void(const safe_vector<Frame>&)> f)
{
	f_frames_update = f;
}

void Window::on_right_click(std::function<void(const size_t, Frame&)> f)
{
	f_rclick = f;
}

const display& Window::get_display() const
{
	return disp;
}

display& Window::get_display()
{
	return disp;
}
