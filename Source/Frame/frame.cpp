#include "frame.h"

using namespace Lunaris;

Frame::Frame(Frame&& oth) noexcept
{
	mname = std::move(oth.mname);
	asblk = std::move(oth.asblk);
}

void Frame::operator=(Frame&& oth) noexcept
{
	mname = std::move(oth.mname);
	asblk = std::move(oth.asblk);
}

bool Frame::load(std::function<hybrid_memory<texture>(void)> f, const std::string& n)
{
	if (n.empty()) return false; // no name
	
	mname = n;
	hybrid_memory<texture> bitmap = f();

	if (!bitmap.valid() || !bitmap->valid()) return false;

	asblk->texture_remove_all();
	asblk->texture_insert(bitmap);

	recenter_auto();
	rescale_auto();

	return true;
}

void Frame::draw() const
{
	asblk->think();
	asblk->draw();
}

const std::string& Frame::name() const
{
	return mname;
}

bool Frame::hit(const int x, const int y, const display& d) const
{
	collisionable cl(*asblk);
	cl.reset();
	return (cl.quick_one_point_overlap(x, y).one_direction != collisionable::direction_index::NONE);
}

void Frame::reset()
{
	asblk = std::make_unique<block>();
}

void Frame::recenter_auto()
{
	ALLEGRO_BITMAP* target = al_get_target_bitmap(); // a display must have one target bitmap afaik
	if (!target) return; // has to have targer.
	const int center_x = 0.5f * al_get_bitmap_width(target); // offset
	const int center_y = 0.5f * al_get_bitmap_height(target); // offset
	asblk->set<float>(enum_sprite_float_e::POS_X, center_x);
	asblk->set<float>(enum_sprite_float_e::POS_Y, center_y);
}

void Frame::rescale_auto()
{
	auto bitmap = asblk->texture_index(0);
	if (bitmap.empty() || bitmap->empty()) return;
	ALLEGRO_BITMAP* target = al_get_target_bitmap(); // a display must have one target bitmap afaik
	if (!target) return; // has to have targer.

	const float coef_x = al_get_bitmap_width(target) * 1.0f / bitmap->get_width(); // scale in x
	const float coef_y = al_get_bitmap_height(target) * 1.0f / bitmap->get_height(); // scale in y

	const float final_scale = ((coef_x < coef_y) ? coef_x : coef_y) * scale(); // the smaller scale * scale

	asblk->set<float>(enum_sprite_float_e::SCALE_X, bitmap->get_width() * final_scale);
	asblk->set<float>(enum_sprite_float_e::SCALE_Y, bitmap->get_height() * final_scale);
	asblk->set<float>(enum_sprite_float_e::SCALE_G, 1.0f);
}

float& Frame::posx()
{
	return asblk->get<float>(enum_sprite_float_e::POS_X);
}

float& Frame::posy()
{
	return asblk->get<float>(enum_sprite_float_e::POS_Y);
}

float& Frame::scale()
{
	return asblk->get<float>(enum_sprite_float_e::SCALE_G);
}

float& Frame::posx() const
{
	return asblk->get<float>(enum_sprite_float_e::POS_X);
}

float& Frame::posy() const
{
	return asblk->get<float>(enum_sprite_float_e::POS_Y);
}

float& Frame::scale() const
{
	return asblk->get<float>(enum_sprite_float_e::SCALE_G);
}
