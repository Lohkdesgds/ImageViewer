#pragma once

#include <string>

class Version {
	std::basic_string<unsigned char> m_interp;
public:
	Version() = default;
	Version(const std::string&);

	void set(std::string);
	std::string get() const;

	bool operator==(const Version&) const;
	bool operator!=(const Version&) const;
	bool operator>(const Version&) const;
	bool operator<(const Version&) const;
	bool operator>=(const Version&) const;
	bool operator<=(const Version&) const;
};