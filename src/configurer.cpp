#include "../include/configurer.h"

std::string get_path(const char* _env)
{
	size_t len{};
	if (getenv_s(&len, nullptr, 0, _env) != 0) return {};
	if (len == 0) return {};

	std::string buf(len, '\0');
	if (getenv_s(&len, buf.data(), buf.size(), _env) != 0) return {};

	buf.pop_back();
	for (auto& i : buf) if (i == '\\') i = '/';
	return buf;
}

std::string _make_work()
{
	const auto final_path = get_path("APPDATA") + config_appdata_path;
	std::filesystem::create_directories(final_path.substr(0, final_path.rfind('/')));
	FILE* fp = nullptr;
	fopen_s(&fp, final_path.c_str(), "ab+");
	fclose(fp);
	return final_path;
}

AllegroCPP::Config& g_conf()
{
	static AllegroCPP::Config conf(_make_work());
	return conf;
}

void g_save_conf()
{
	g_conf().save(_make_work());
}
