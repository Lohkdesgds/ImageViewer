#pragma once

#include <Lunaris/all.h>
#include "shared.h"

using namespace Lunaris;

#define IMGVW_STANDARD_START_CONFIGURATION Lunaris::console::color::AQUA << "[CONFIG] " << Lunaris::console::color::GRAY 

namespace ImageViewer {

	class ConfigManager {
		config conf;
		std::string conf_path;
	public:
		ConfigManager();

		const config& read_conf() const;
		config& write_conf();

	};

}