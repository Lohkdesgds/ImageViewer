#include <Lunaris/all.h>

#include "Window/window.h"
#include "MenuCtl/menuctl.h"

using namespace Lunaris;

int main(int argc, char* argv[])
{
	set_app_name("Lohk's Studios/Image Viewer");

	std::vector<std::string> arguments;
	for (int a = 1; a < argc; a++) {
		std::string mv = argv[a];
		if (std::regex_search(mv, regex_link, std::regex_constants::match_any)) {
			arguments.emplace_back(std::move(mv));
			continue;
		}

		file fp;
		if (fp.open(mv, file::open_mode_e::READ_TRY)) {
			arguments.emplace_back(std::move(mv));
		}
	}

	cout << "Detected new list of items: " << arguments.size() << " item(s)";
	for (const auto& it : arguments) cout << "- '" << it << "'";

	Window window;
	MenuCtl menn(window.get_display());
	
	for (const auto& it : arguments) window.push(it);// .then([](const bool s) { if (s) { cout << "Success opening file."; } else { cout << "Failed opening file."; } });

	window.on_frames_update([&menn, &window](const safe_vector<Frame>& v) { menn.update_frames(v); });
	window.on_right_click([&menn, &window](const size_t& index, Frame& fr) { menn.show_pop(); });

	menn.on_frame_select([&menn, &window](size_t p) { window.put_on_top(p); });
	menn.on_event([&](menu_event& mev) {
		switch (mev.get_id()) {
		case +e_menu_item::RC_CENTER:
			window.post_right_click([](Frame& fm) {fm.recenter_auto(); });
			break;
		case +e_menu_item::RC_RESET:
			window.post_right_click([](Frame& fm) {fm.recenter_auto(); fm.rescale_auto(); });
			break;
		}
	});
	menn.show_bar();

	while (window.draw());

}