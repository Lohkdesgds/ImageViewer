#pragma once

#include <config.h>
#include <filesystem>

const std::string config_appdata_path = "/Lohk's Studios/ImageViewer/config_v5.ini";

std::string get_path(const char* _env);

AllegroCPP::Config& g_conf();
void g_save_conf();