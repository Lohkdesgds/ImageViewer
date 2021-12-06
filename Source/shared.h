#pragma once

#include <string>
#include <regex>

const std::string versioning = "v4.0.0";
const unsigned long long delta_time_seconds_next_check = 3600; // each hour
const std::string url_update_check = "https://api.github.com/repos/Lohkdesgds/ImageViewer/releases/latest";
const std::string url_update_popup = "https://github.com/Lohkdesgds/ImageViewer/releases";


const std::regex regex_link("http(?:s?)://([\\w_-]+(?:(?:\\.[\\w_-]+)+))([\\w.,@?^=%&:/~+#-]*[\\w@?^=%&/~+#-])?");
const std::string __gif_default_start = "GIF89a";
const std::string png_format = { (char)137, (char)80, (char)78, (char)71, (char)13, (char)10, (char)26, (char)10 }; // http://www.libpng.org/pub/png/spec/1.2/PNG-Structure.html

const size_t max_item_name_size = 50;