#include <LSWv5.h>

using namespace LSW::v5;
using namespace LSW::v5::Interface;

constexpr int minimum_scale = -45;
constexpr int maximum_scale = 1000;

int main(int argc, char* argv[]) {

	Logger logg;
	EventHandler handling{ Tools::superthread::performance_mode::HIGH_PERFORMANCE };
	Display disp{ 0 };
	std::vector<std::string> parsed;
	EventTimer timer{ 1.0 / 60 };

	bool is_mouse_pressed = false;
	int moved[2] = { 0,0 };


	logg.init("%appdata%/Lohk's Studios/Image Viewer/log.log");

	logg << L::SLF << fsr() << "Loading ImageViewer based on LSW v5..." << L::ELF;

	disp.set_vsync(true);
	disp.hide_mouse(false);
	disp.set_fullscreen(true);
	disp.set_new_flags(Interface::display::default_new_display_flags | ALLEGRO_FULLSCREEN_WINDOW);

	logg << L::SLF << fsr() << "Parsing parameters..." << L::ELF;

	for (int u = 1; u < argc; u++) {
		if (auto siz = Interface::quick_get_file_size(argv[u]); siz > 0) {
			logg << L::SLF << fsr() << "Added '" << argv[u] << "' to the list (size: " << Tools::byte_auto_string(siz) << "iB)." << L::ELF;
			parsed.push_back(argv[u]);
		}
		else {
			logg << L::SLF << fsr(E::WARN) << "Failed to add " << argv[u] << " to the list (invalid size?)." << L::ELF;
		}
	}

	if (!parsed.size()) {
		logg << L::SLF << fsr(E::ERRR) << "Not enough arguments or invalid ones. Please try dragging and dropping the image file onto the app." << L::ELF;
		std::this_thread::sleep_for(std::chrono::seconds(10));
		return 1;
	}



	logg << L::SLF << fsr() << "Initializing display..." << L::ELF;

	auto ended = disp.init();

	while (!disp.display_ready()) std::this_thread::yield();

	handling.add(disp.get_event_source());
	handling.add(get_keyboard_event());
	handling.add(get_mouse_event());
	handling.add(timer);

	timer.start();


	size_t index = 0;
	size_t last_draw_func = 0;
	int scale = 0; // offset
	bool scale_update = false;
	

	handling.set_run_autostart([&](const Interface::RawEvent& ev) {

		switch (ev.type()) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			logg << L::SLF << fsr() << "Got event to close the app. Closing..." << L::ELF;
			disp.set_stop();
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			is_mouse_pressed = true;
			disp.hide_mouse(true);
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			is_mouse_pressed = false;
			disp.hide_mouse(false);
			break;
		case ALLEGRO_EVENT_MOUSE_AXES:
			if (is_mouse_pressed) {
				moved[0] += 2 * ev.mouse_event().dx;
				moved[1] += 2 * ev.mouse_event().dy;
				disp.move_mouse_to(0.0, 0.0);
			}
			else {
				disp.hide_mouse(false);
			}
			if (ev.mouse_event().dz) {
				scale += 3 * ev.mouse_event().dz;
				scale_update = true;
			}
			break;
		case ALLEGRO_EVENT_TIMER:
			if (ev.timer_event().source == timer && (is_mouse_pressed || scale_update)) {
				
				if (scale <= minimum_scale) scale = minimum_scale;
				if (scale >= maximum_scale) scale = maximum_scale;

				disp.get_current_camera().get_classic().x -= 0.7f * moved[0] * pow(1.0f / (scale - minimum_scale + 10), 2.0f);
				disp.get_current_camera().get_classic().y -= 0.7f * moved[1] * pow(1.0f / (scale - minimum_scale + 10), 2.0f);
				disp.get_current_camera().get_classic().sx = pow(10.0f + 0.1f * scale, 4.0f) / 10e3f;
				disp.get_current_camera().get_classic().sy = pow(10.0f + 0.1f * scale, 4.0f) / 10e3f;
				scale_update = false;

				if (moved[0] != 0 || moved[1] != 0 || scale_update)
					logg << L::SLF << fsr() << "Position/scale changed: {" 
					<< FormatAs("%.3f") << disp.get_current_camera().get_classic().x << ";"
					<< FormatAs("%.3f") << disp.get_current_camera().get_classic().y << ";Z="
					<< FormatAs("%.4f") << disp.get_current_camera().get_classic().sx << "}" << L::ELF;

				disp.set_refresh_camera();
				moved[0] = moved[1] = 0;
			}
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
			switch (ev.keyboard_event().keycode) {
			case ALLEGRO_KEY_R:
				disp.get_current_camera().get_classic().x = 0.0f;
				disp.get_current_camera().get_classic().y = 0.0f;
				disp.get_current_camera().get_classic().sx = disp.get_current_camera().get_classic().sy = 1.0f;
				scale = 0;
				scale_update = true;
				break;
			case ALLEGRO_KEY_ESCAPE:
				logg << L::SLF << fsr() << "Got ESCAPE to close the app. Closing..." << L::ELF;
				disp.set_stop();
				break;
			case ALLEGRO_KEY_LEFT:
				if (parsed.size()) {
					if (index) index--;
					else index = parsed.size() - 1;
				}
				else index = 0;

				disp.get_current_camera().get_classic().x = 0.0f;
				disp.get_current_camera().get_classic().y = 0.0f;
				disp.get_current_camera().get_classic().sx = disp.get_current_camera().get_classic().sy = 1.0f;
				scale = 0;
				scale_update = true;

				disp.hide_mouse(true);
				break;
			case ALLEGRO_KEY_RIGHT:
				if (++index >= parsed.size()) index = 0;
				// "automatic else"

				disp.get_current_camera().get_classic().x = 0.0f;
				disp.get_current_camera().get_classic().y = 0.0f;
				disp.get_current_camera().get_classic().sx = disp.get_current_camera().get_classic().sy = 1.0f;
				scale = 0;
				scale_update = true;

				disp.hide_mouse(true);
				break;
			}
			break;
		}
	});


	Work::Block draww;
	draww.set(Work::sprite::e_double::DISTANCE_DRAWING_SCALE, 1e300); // sincerely, any.
	Interface::Bitmap bmp, tempbmp;

	draww.set(Work::sprite::e_boolean::DRAW, true);
	draww.set(Work::sprite::e_double::SCALE_G, 2.0);
	draww.set(Work::sprite::e_boolean::AFFECTED_BY_CAM, true);

	while (!ended.get_ready()) {

		if (!parsed.size()) {
			logg << L::SLF << fsr(E::ERRR) << "No possible images to show! Did you drop only non-images? Closing the app..." << L::ELF;
			disp.set_stop();
			break;
		}
		else if (index >= parsed.size()) {
			index = 0;
		}

		size_t last_index = index;

		// not black flash screen
		size_t _wait_task = disp.add_draw_task([](const Camera& c) {
			Color test{ 127,127,127 };
			test.clear_to_this();
		});


		auto sure = disp.add_once_task([&] {
			return tempbmp.load(parsed[index].c_str());
		});

		sure.wait();

		if (sure.get().get<bool>()) {
			logg << L::SLF << fsr() << "Loaded '" << parsed[index] << "'. Computing and previewing..." << L::ELF;

			double proportion = tempbmp.get_width() * 1.0 / tempbmp.get_height();
			double display_prop = disp.get_width() * 1.0 / disp.get_height();

			// if disp 1, bmp 1, res = 1
			// if disp 1, bmp 0.5 (height double width), x has to be 0.5
			// if disp 1, bmp 2 (width double height), y has to be 1.0 / 2

			// if disp 2 (width double height), bmp 1, x has to be 0.5, so bmp / disp!
			
			double result_prop = proportion * 1.0 / display_prop;

			if (result_prop < 1.0) {
				draww.set(Work::sprite::e_double::SCALE_X, result_prop);
				draww.set(Work::sprite::e_double::SCALE_Y, 1.0);
			}
			else {
				draww.set(Work::sprite::e_double::SCALE_X, 1.0);
				draww.set(Work::sprite::e_double::SCALE_Y, 1.0 / result_prop);
			}

			if (disp.remove_draw_task(last_draw_func)) {
				logg << L::SLF << fsr() << "Cleaned up last task." << L::ELF;
			}

			draww.remove([](const Interface::Bitmap& b) { return true; }); // remove all lmao
			bmp.reset();
			bmp = std::move(tempbmp);
			draww.insert(bmp);

			last_draw_func = disp.add_draw_task([&draww](const Interface::Camera& cam) {
				draww.draw(cam);
			});
		}
		else {
			logg << L::SLF << fsr(E::WARN) << "Could not load '" << parsed[index] << "' properly (is this an image?). Removing from list and skipping." << L::ELF;
			parsed.erase(parsed.begin() + index);
			continue;
		}

		disp.remove_draw_task(_wait_task);

		while (last_index == index && !ended.get_ready()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			std::this_thread::yield();
		}
	}

	if (!ended.get()) {
		logg << L::SLF << fsr(E::ERRR) << "Something went wrong with the app. Please contact support." << L::ELF;
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));
	handling.stop();
	disp.deinit();

}