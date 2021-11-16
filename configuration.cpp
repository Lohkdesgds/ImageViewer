#include "configuration.h"

namespace ImageViewer {

	ConfigManager::ConfigManager()
	{
		cout << IMGVW_STANDARD_START_CONFIGURATION << "Loading ConfigManager...";

		cout << IMGVW_STANDARD_START_CONFIGURATION << "Setting up environment app name...";
		set_app_name("Lohk's Studios/Image Viewer");

		cout << IMGVW_STANDARD_START_CONFIGURATION << "Generating configuration path...";
		auto root_path = fix_path_to(get_standard_path());
		make_path(root_path);

		conf_path = root_path + "last.conf";

		if (!conf.load(conf_path)) {
			cout << IMGVW_STANDARD_START_CONFIGURATION << console::color::YELLOW << "Can't find file or failed to load. Creating a new one instead.";
		}

		cout << IMGVW_STANDARD_START_CONFIGURATION << "Ensuring variables...";

		conf.ensure("general", "was_fullscreen", false, config::config_section_mode::SAVE);
		conf.ensure("general", "smooth_scale", true, config::config_section_mode::SAVE);
		//conf.ensure("general", "was_rightclick", false, config::config_section_mode::SAVE);
		conf.ensure("general", "width", 1280, config::config_section_mode::SAVE);
		conf.ensure("general", "height", 720, config::config_section_mode::SAVE);
		conf.ensure("general", "last_time_version_check", 0, config::config_section_mode::SAVE);

		conf.ensure("general", "had_update_last_time", false, config::config_section_mode::SAVE); // has_update
		conf.ensure("general", "last_version_check", versioning, config::config_section_mode::SAVE); // tag_name

		conf.ensure<std::string>("general", "last_seen_list", {}, config::config_section_mode::SAVE);

		cout << IMGVW_STANDARD_START_CONFIGURATION << console::color::GREEN << "Ready.";
	}

	const config& ConfigManager::read_conf() const
	{
		return conf;
	}

	config& ConfigManager::write_conf()
	{
		return conf;
	}

}