#pragma once

#include <string>
#include <memory>
#include <filesystem>

#include <HTTP/http.h>
#include <nlohmann/json.hpp>
#include <System.h>

#include "versioning.h"

const std::string url_update_check = "https://api.github.com/repos/Lohkdesgds/ImageViewer/releases/latest";
const std::string url_update_popup = "https://github.com/Lohkdesgds/ImageViewer/releases";

const auto current_version = Version("5.0.0");

class Updater {
	struct data {
		const std::string remote_version;
		const std::string remote_title;
		const std::string remote_body;
		const std::string remote_link;
		const Version remote_version_parsed;

		data(const std::string&, const std::string&, const std::string&, const std::string&);
	};

	std::unique_ptr<data> m_data; // generated on fetch
public:
	bool fetch();
	bool has_update() const;
	bool is_up_to_date() const;

	bool valid() const;
	const data& get() const;
};

std::string http_get(const std::string&);