#include "frame.h"

using namespace Lunaris;

Frame::Frame(Frame&& oth) noexcept
{
	bitmap = oth.bitmap;
	mname = std::move(oth.mname);
	posx = oth.posx;
	posy = oth.posy;
	scale = oth.scale;
}

void Frame::operator=(Frame&& oth) noexcept
{
	bitmap = oth.bitmap;
	mname = std::move(oth.mname);
	posx = oth.posx;
	posy = oth.posy;
	scale = oth.scale;
}

bool Frame::load(std::function<hybrid_memory<texture>(void)> f, const std::string& n)
{
	if (n.empty()) return false; // no name
	
	mname = n;
	bitmap = f();
	return bitmap.valid() && bitmap->valid();
}

void Frame::draw() const
{
	// draw
	ALLEGRO_BITMAP* target = al_get_target_bitmap(); // a display must have one target bitmap afaik
	if (target && bitmap.valid() && bitmap->valid())
	{
		const int center_x = 0.5f * al_get_bitmap_width(target); // offset
		const int center_y = 0.5f * al_get_bitmap_height(target); // offset

		const float coef_x = al_get_bitmap_width(target) * 1.0f / bitmap->get_width(); // scale in x
		const float coef_y = al_get_bitmap_height(target) * 1.0f / bitmap->get_height(); // scale in y

		const float final_scale = ((coef_x < coef_y) ? coef_x : coef_y) * scale; // the smaller scale * scale

		// draw scale to fit screen, scale var does offset, px and py are center related offset
		bitmap->draw_scaled_rotated_at(
			0.0f,
			0.0f,
			center_x + posx,
			center_y + posy,
			final_scale,
			final_scale,
			0.0f);
	}
}

const std::string& Frame::name() const
{
	return mname;
}

bool Frame::hit(const int x, const int y, const display& d) const
{
	const int center_x = 0.5f * d.get_width(); // offset
	const int center_y = 0.5f * d.get_height(); // offset

	const float coef_x = d.get_width() * 1.0f / bitmap->get_width(); // scale in x
	const float coef_y = d.get_height() * 1.0f / bitmap->get_height(); // scale in y

	const float final_scale = ((coef_x < coef_y) ? coef_x : coef_y) * scale; // the smaller scale * scale

	const int left = center_x + posx - 0.5f * bitmap->get_width() * final_scale;
	const int right = center_x + posx + 0.5f * bitmap->get_width() * final_scale;
	const int top = center_y + posy - 0.5f * bitmap->get_height() * final_scale;
	const int bottom = center_y + posy + 0.5f * bitmap->get_height() * final_scale;

	return ((x >= left && x <= right) && (y >= top && y <= bottom));
}

void Frame::reset()
{
	bitmap.reset_this();
}