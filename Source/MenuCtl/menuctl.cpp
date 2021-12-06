#include "menuctl.h"

using namespace Lunaris;

std::vector<menu_quick> gen_menu_bar()
{
	return {
		menu_each_menu()
		.set_caption("File")
		.set_id(+e_menu_item::FILE)
		.push(menu_each_menu()
			.set_caption("Layers")
			.set_id(+e_menu_item::LAYERS_ROOT)
		)
	};
}

std::vector<menu_quick> gen_menu_pop()
{
	return {
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
	};
}

MenuCtl::MenuCtl()
	: mn_bar(menu::menu_type::BAR), mn_pop(menu::menu_type::POPUP)
{
	mn_bar.push(gen_menu_bar());
	mn_pop.push(gen_menu_pop());
}

void MenuCtl::update_frames(const safe_vector<Frame>& f)
{
	auto fnd1 = mn_bar.find_id(+e_menu_item::LAYERS_ROOT);
	auto fnd2 = mn_pop.find_id(+e_menu_item::LAYERS_ROOT);

	cout << "# Updating menus...";

	while (fnd1.size()) fnd1.remove(0);
	while (fnd2.size()) fnd2.remove(0);

	f.csafe([&](const std::vector<Frame>& fms) {

		int p = 0;
		for (const auto& ff : fms) {

			fnd1.push(menu_each_default().set_id(static_cast<uint16_t>(p)).set_caption("[" + std::to_string(p) + "] " + ff.name()));
			fnd2.push(menu_each_default().set_id(static_cast<uint16_t>(p)).set_caption("[" + std::to_string(p) + "] " + ff.name()));

			cout << "#" << p << " Added " << ff.name() << " to menus.";

			++p;
		}

	});
}

void MenuCtl::show_pop(const display& d)
{
	mn_pop.show(d.get_raw_display());
}

void MenuCtl::show_bar(const display& d)
{
	mn_bar.show(d.get_raw_display());
}

void MenuCtl::hide_bar(const display& d)
{
	mn_bar.hide(d.get_raw_display());
}