#include "menuctl.h"

using namespace Lunaris;

/*
	FILE,
		FILE_OPEN,
		FILE_OPEN_FOLDER,
		FILE_OPEN_URL_CLIPBOARD,
	LAYERS_ROOT,
	WINDOW,
		WINDOW_FRAMELESS,
		WINDOW_NOBAR,

	RC_WINDOW
		RC_WINDOW_FRAMELESS,
		RC_WINDOW_NOBAR,
	RC_ROTATE,
		RC_ROTATE_RIGHT,
		RC_ROTATE_LEFT
*/

std::vector<menu_quick> gen_menu_bar()
{
	return {
		menu_each_menu()
			.set_caption("&File")
			.set_id(+e_menu_item::FILE)
			.push(menu_each_default().set_caption("&Open...").set_id(+e_menu_item::FILE_OPEN))
			.push(menu_each_default().set_caption("Open Fol&der...").set_id(+e_menu_item::FILE_OPEN))
			.push(menu_each_default().set_caption("Open from &clipboard...").set_id(+e_menu_item::FILE_OPEN))
			.push(menu_each_empty())
			.push(menu_each_default().set_caption("&Exit application").set_id(+e_menu_item::FILE_EXIT_APP))
		,
		menu_each_menu()
			.set_caption("&Layers")
			.set_id(+e_menu_item::LAYERS_ROOT)
		,
		menu_each_menu()
			.set_caption("&Window")
			.set_id(+e_menu_item::WINDOW)
			.push(menu_each_default().set_caption("Fra&meless").set_id(+e_menu_item::WINDOW_FRAMELESS).set_flags(menu_flags::AS_CHECKBOX))
			.push(menu_each_default().set_caption("&Bar menu").set_id(+e_menu_item::WINDOW_NOBAR).set_flags(menu_flags::AS_CHECKBOX|menu_flags::CHECKED))
			.push(menu_each_default().set_caption("&Fullscreen").set_id(+e_menu_item::WINDOW_FULLSCREEN).set_flags(menu_flags::AS_CHECKBOX))
	};
}

std::vector<menu_quick> gen_menu_pop()
{
	return {
		menu_each_menu()
			.set_caption("Window")
			.set_id(+e_menu_item::RC_WINDOW)
			.push(menu_each_default().set_caption("Frameless").set_id(+e_menu_item::RC_WINDOW_FRAMELESS).set_flags(menu_flags::AS_CHECKBOX))
			.push(menu_each_default().set_caption("Bar menu").set_id(+e_menu_item::RC_WINDOW_NOBAR).set_flags(menu_flags::AS_CHECKBOX|menu_flags::CHECKED))
			.push(menu_each_default().set_caption("Fullscreen").set_id(+e_menu_item::RC_WINDOW_FULLSCREEN).set_flags(menu_flags::AS_CHECKBOX))
		,
		menu_each_menu()
			.set_caption("Layers")
			.set_id(+e_menu_item::RC_LAYERS_ROOT)
		,
		menu_each_menu()
			.set_caption("Rotate")
			.set_id(+e_menu_item::RC_ROTATE)
			.push(menu_each_menu()
				.set_caption("+90 degrees [todo]")
				.set_id(+e_menu_item::RC_ROTATE_RIGHT)
			)
			.push(menu_each_menu()
				.set_caption("-90 degrees [todo]")
				.set_id(+e_menu_item::RC_ROTATE_LEFT)
			)
		,
		menu_each_default().set_caption("Exit").set_id(+e_menu_item::RC_EXIT)
	};
}

void MenuCtl::on_menu(menu_event& ev)
{
	switch (ev.get_id()) {
	case +e_menu_item::WINDOW_FRAMELESS:
	case +e_menu_item::RC_WINDOW_FRAMELESS:
		{
			auto f1 = mn_bar.find_id(+e_menu_item::WINDOW_FRAMELESS);
			auto f2 = mn_pop.find_id(+e_menu_item::RC_WINDOW_FRAMELESS);

			if (static_cast<bool>(f1.get_flags() & menu_flags::CHECKED) == static_cast<bool>(disp.get_flags() & ALLEGRO_FRAMELESS))
				f1.toggle_flags(menu_flags::CHECKED);
			if (static_cast<bool>(f2.get_flags() & menu_flags::CHECKED) == static_cast<bool>(disp.get_flags() & ALLEGRO_FRAMELESS))
				f2.toggle_flags(menu_flags::CHECKED);
		}
		disp.toggle_flag(ALLEGRO_FRAMELESS);
		break;
	case +e_menu_item::WINDOW_NOBAR:
	case +e_menu_item::RC_WINDOW_NOBAR:
		if (is_bar_enabled) {
			hide_bar();
		}
		else {
			show_bar();
		}
		{
			auto f1 = mn_bar.find_id(+e_menu_item::WINDOW_NOBAR);
			auto f2 = mn_pop.find_id(+e_menu_item::RC_WINDOW_NOBAR);

			if (static_cast<bool>(f1.get_flags() & menu_flags::CHECKED) != is_bar_enabled)
				f1.toggle_flags(menu_flags::CHECKED);
			if (static_cast<bool>(f2.get_flags() & menu_flags::CHECKED) != is_bar_enabled)
				f2.toggle_flags(menu_flags::CHECKED);
		}
		break;
	case +e_menu_item::WINDOW_FULLSCREEN:
	case +e_menu_item::RC_WINDOW_FULLSCREEN:
		{
			auto f1 = mn_bar.find_id(+e_menu_item::WINDOW_FULLSCREEN);
			auto f2 = mn_pop.find_id(+e_menu_item::RC_WINDOW_FULLSCREEN);
			auto f3 = mn_bar.find_id(+e_menu_item::WINDOW_FRAMELESS); // disabled when fullscreen
			auto f4 = mn_pop.find_id(+e_menu_item::RC_WINDOW_FRAMELESS); // disabled when fullscreen

			const bool was_fullscreen = disp.get_flags() & ALLEGRO_FULLSCREEN_WINDOW;
			const bool was_barr = is_bar_enabled;

			if (static_cast<bool>(f1.get_flags() & menu_flags::CHECKED) == was_fullscreen)
				f1.toggle_flags(menu_flags::CHECKED);
			if (static_cast<bool>(f2.get_flags() & menu_flags::CHECKED) == was_fullscreen)
				f2.toggle_flags(menu_flags::CHECKED);

			if (was_fullscreen) {
				f3.unset_flags(menu_flags::DISABLED);
				f4.unset_flags(menu_flags::DISABLED);
			}
			else {
				hide_bar();
				f3.set_flags(menu_flags::DISABLED);
				f4.set_flags(menu_flags::DISABLED);
			}

			disp.toggle_flag(ALLEGRO_FULLSCREEN_WINDOW);
			if (was_barr) disp.post_task([&] { show_bar(); return true; });
		}
		break;
	case +e_menu_item::FILE_EXIT_APP:
	case +e_menu_item::RC_EXIT:
		disp.destroy();
	}
}

MenuCtl::MenuCtl(display& d)
	: mn_bar(menu::menu_type::BAR), mn_pop(menu::menu_type::POPUP), disp(d)
{
	mn_bar.push(gen_menu_bar());
	mn_pop.push(gen_menu_pop());

	mn_evhlr.install_other(mn_bar);
	mn_evhlr.install_other(mn_pop);
	mn_evhlr.hook_event_handler([this](const ALLEGRO_EVENT& rev) { menu_event ev(rev.any.source == mn_bar.get_event_source() ? mn_bar : mn_pop, rev); on_menu(ev); });
}

void MenuCtl::update_frames(const safe_vector<Frame>& f)
{
	auto fnd1 = mn_bar.find_id(+e_menu_item::LAYERS_ROOT);
	auto fnd2 = mn_pop.find_id(+e_menu_item::RC_LAYERS_ROOT);

	cout << "# Updating menus...";

	fnd1.remove_all();
	fnd2.remove_all();


	f.csafe([&](const std::vector<Frame>& fms) {

		int p = 1;
		for (const auto& ff : fms) {

			fnd1.push(menu_each_default().set_id(static_cast<uint16_t>(p)).set_caption(std::to_string(p) + " > " + ff.name()));
			fnd2.push(menu_each_default().set_id(static_cast<uint16_t>(p)).set_caption(std::to_string(p) + " > " + ff.name()));

			cout << "#" << p << " Added " << ff.name() << " to menus.";

			++p;
		}

	});
}

void MenuCtl::show_pop()
{
	if (disp.valid()) mn_pop.show(disp.get_raw_display());
}

void MenuCtl::show_bar()
{
	if (!is_bar_enabled && disp.valid()) {
		is_bar_enabled = true;
		mn_bar.show(disp.get_raw_display());
	}
}

void MenuCtl::hide_bar()
{
	if (is_bar_enabled && disp.valid()) {
		is_bar_enabled = false;
		mn_bar.hide(disp.get_raw_display());
	}
}