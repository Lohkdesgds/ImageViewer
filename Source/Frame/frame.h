#pragma once

#include <Lunaris/all.h>

using namespace Lunaris;

class Frame : public NonCopyable {
	std::string mname;
	std::unique_ptr<block> asblk = std::make_unique<block>();
public:
	Frame() = default;
	Frame(Frame&&) noexcept;
	void operator=(Frame&&) noexcept;

	bool load(std::function<hybrid_memory<texture>(void)>, const std::string&);
	void draw() const;
	const std::string& name() const;

	bool hit(const int, const int, const display&) const;

	void reset();

	void recenter_auto();
	void rescale_auto();

	float& posx();
	float& posy();
	float& scale();
	float& posx() const;
	float& posy() const;
	float& scale() const;
};