#include "../include/updater.h"

std::string http_get(const std::string& url) { std::string buf; Lunaris::HttpSimple().Get(url, buf); return buf; }

Updater::data::data(const std::string& a, const std::string& b, const std::string& c, const std::string& d)
	: remote_version(a), remote_title(b), remote_body(c), remote_link(d), remote_version_parsed(a)
{
}

bool Updater::fetch()
{
	try {
		const auto dat = http_get(url_update_check);
		if (dat.empty()) return false;

		const auto jlatest = nlohmann::json::parse(dat, nullptr, false);
		if (jlatest.is_discarded()) return false;

		m_data = std::make_unique<data>(jlatest["tag_name"], jlatest["name"], jlatest["body"], jlatest["html_url"]);

		return (m_data->remote_version_parsed > current_version);
	}
	catch (...) {
		return false;
	}
}

bool Updater::has_update() const
{
	return m_data && (m_data->remote_version_parsed > current_version);
}

bool Updater::is_up_to_date() const
{
	return !m_data || (m_data->remote_version_parsed == current_version);
}

bool Updater::valid() const
{
	return m_data.get();
}

const Updater::data& Updater::get() const
{
	return *m_data.get();
}
