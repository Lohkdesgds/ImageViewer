#include "../include/versioning.h"

#include <sstream>

Version::Version(const std::string& s)
{
	set(s);
}

void Version::set(std::string s)
{
	while (s.size() && s.front() > '9' || s.front() < '0') s.erase(s.begin());

	std::istringstream ss(s);

	std::string token;
	size_t counter = 0;
	while (std::getline(ss, token, '.')) {
		uint8_t num = static_cast<uint8_t>(std::stoi(token));
		m_interp += (unsigned char)num;
	}
}

std::string Version::get() const
{
	std::string out;
	for (const auto& i : m_interp) {
		out += std::to_string(static_cast<uint32_t>(i)) + '.';
	}
	if (out.size()) out.pop_back();
	return out;
}

bool Version::operator==(const Version& o) const
{
	return o.m_interp == m_interp;
}

bool Version::operator!=(const Version& o) const
{
	return o.m_interp != m_interp;
}

bool Version::operator>(const Version& o) const
{
	if (o.m_interp.size() == m_interp.size()) return memcmp(m_interp.data(), o.m_interp.data(), m_interp.size()) > 0;

	auto d1 = m_interp;
	auto d2 = o.m_interp;

	if (d1.size() < d2.size()) d2.resize(d1.size());
	if (d2.size() < d1.size()) d1.resize(d2.size());

	return memcmp(d1.data(), d2.data(), d1.size()) > 0;

	//for (size_t p = 0; p < o.m_interp.size() && p < m_interp.size(); ++p)
	//{
	//	if (m_interp[p] > o.m_interp[p]) return true;
	//}
	//return m_interp.size() > o.m_interp.size();
}

bool Version::operator<(const Version& o) const
{
	if (o.m_interp.size() == m_interp.size()) return memcmp(m_interp.data(), o.m_interp.data(), m_interp.size()) < 0;

	auto d1 = m_interp;
	auto d2 = o.m_interp;

	if (d1.size() < d2.size()) d2.resize(d1.size());
	if (d2.size() < d1.size()) d1.resize(d2.size());

	return memcmp(d1.data(), d2.data(), d1.size()) < 0;

	//for (size_t p = 0; p < o.m_interp.size() && p < m_interp.size(); ++p)
	//{
	//	if (m_interp[p] < o.m_interp[p]) return true;
	//}
	//return m_interp.size() < o.m_interp.size();
}

bool Version::operator>=(const Version& o) const
{
	return *this == o || *this > o;
}

bool Version::operator<=(const Version& o) const
{
	return *this == o || *this < o;
}
