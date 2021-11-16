#pragma once

#include <string>

namespace ImageViewer {

	const std::string versioning = "v3.0.0";
	const unsigned long long delta_time_seconds_next_check = 3600; // each hour
	const std::string url_update_check = "https://api.github.com/repos/Lohkdesgds/ImageViewer/releases/latest";
	const std::string url_update_popup = "https://github.com/Lohkdesgds/ImageViewer/releases";

}