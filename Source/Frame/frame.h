#pragma once

#include <Lunaris/all.h>

using namespace Lunaris;

class Frame : public NonCopyable {
	hybrid_memory<texture> bitmap;
	std::string mname;
public:
	int posx = 0;
	int posy = 0;
	float scale = 1.0f;

	Frame() = default;
	Frame(Frame&&) noexcept;
	void operator=(Frame&&) noexcept;

	bool load(std::function<hybrid_memory<texture>(void)>, const std::string&);
	void draw() const;
	const std::string& name() const;

	void reset();
};