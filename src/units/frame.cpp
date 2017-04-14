/*
   Copyright (C) 2006 - 2017 by Jeremy Rosen <jeremy.rosen@enst-bretagne.fr>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "units/frame.hpp"

#include "color.hpp"
#include "game_display.hpp"
#include "log.hpp"
#include "sound.hpp"

static lg::log_domain log_engine("engine");
#define ERR_NG LOG_STREAM(err, log_engine)

template<>
frame_parameters::basic_frame_parameters()
	: duration(0)
	, halo_x(0)
	, halo_y(0)
	, blend_ratio(0.0)
	, highlight_ratio(1.0)
	, offset(0)
	, submerge(0.0)
	, x(0)
	, y(0)
	, directional_x(0)
	, directional_y(0)
	, auto_vflip(boost::logic::indeterminate)
	, auto_hflip(boost::logic::indeterminate)
	, primary_frame(boost::logic::indeterminate)
	, drawing_layer(display::LAYER_UNIT_DEFAULT - display::LAYER_UNIT_FIRST)
{}

template<>
builder_frame_parameters::basic_frame_parameters()
	: duration(1)
	, auto_vflip(boost::logic::indeterminate)
	, auto_hflip(boost::logic::indeterminate)
	, primary_frame(boost::logic::indeterminate)
	, drawing_layer(std::to_string(display::LAYER_UNIT_DEFAULT - display::LAYER_UNIT_FIRST))
{}

frame_builder::frame_builder()
	: builder_frame_parameters()
{}

frame_builder::frame_builder(const config& cfg,const std::string& frame_string)
	: builder_frame_parameters()
{
	using base = builder_frame_parameters;
	base::image = cfg[frame_string + "image"].str();
	base::image_diagonal = cfg[frame_string + "image_diagonal"].str();
	base::image_mod = cfg[frame_string + "image_mod"].str();
	base::halo = cfg[frame_string + "halo"].str();
	base::halo_x = cfg[frame_string + "halo_x"].str();
	base::halo_y = cfg[frame_string + "halo_y"].str();
	base::halo_mod = cfg[frame_string + "halo_mod"].str();
	base::sound = cfg[frame_string + "sound"].str();
	base::text = cfg[frame_string + "text"].str();
	base::blend_ratio = cfg[frame_string + "blend_ratio"].str();
	base::highlight_ratio = cfg[frame_string + "alpha"].str();
	base::offset = cfg[frame_string + "offset"].str();
	base::submerge = cfg[frame_string + "submerge"].str();
	base::x = cfg[frame_string + "x"].str();
	base::y = cfg[frame_string + "y"].str();
	base::directional_x = cfg[frame_string + "directional_x"].str();
	base::directional_y = cfg[frame_string + "directional_y"].str();
	base::drawing_layer = cfg[frame_string + "layer"].str();
	
	if(!cfg.has_attribute(frame_string + "auto_vflip")) {
		base::auto_vflip = boost::logic::indeterminate;
	} else {
		base::auto_vflip = cfg[frame_string + "auto_vflip"].to_bool();
	}

	if(!cfg.has_attribute(frame_string + "auto_hflip")) {
		base::auto_hflip = boost::logic::indeterminate;
	} else {
		base::auto_hflip = cfg[frame_string + "auto_hflip"].to_bool();
	}

	if(!cfg.has_attribute(frame_string + "primary")) {
		base::primary_frame = boost::logic::indeterminate;
	} else {
		base::primary_frame = cfg[frame_string + "primary"].to_bool();
	}

	std::vector<std::string> color = utils::split(cfg[frame_string + "text_color"]);
	if(color.size() == 3) {
		try {
			text_color = color_t(std::stoi(color[0]), std::stoi(color[1]), std::stoi(color[2]));
		} catch(std::invalid_argument) {
			ERR_NG << "Invalid RGB color value in unit animation: " << color[0] << ", " << color[1] << ", " << color[2] << "\n";
		}
	}

	if(const config::attribute_value* v = cfg.get(frame_string + "duration")) {
		duration(*v);
	} else if(!cfg.get(frame_string + "end")) {
		int halo_duration = (progressive_string(base::halo, 1)).duration();
		int image_duration = (progressive_image(base::image, 1)).duration();
		int image_diagonal_duration = (progressive_image(base::image_diagonal, 1)).duration();

		duration(std::max(std::max(image_duration, image_diagonal_duration), halo_duration));
	} else {
		duration(cfg[frame_string + "end"].to_int() - cfg[frame_string + "begin"].to_int());
	}

	base::duration = std::max(base::duration, 1);

	color = utils::split(cfg[frame_string + "blend_color"]);
	if(color.size() == 3) {
		try {
			base::blend_with = color_t(std::stoi(color[0]), std::stoi(color[1]), std::stoi(color[2]));
		} catch(std::invalid_argument) {
			ERR_NG << "Invalid RGB color value in unit animation: " << color[0] << ", " << color[1] << ", " << color[2] << "\n";
		}
	}
}

frame_builder& frame_builder::image(const std::string& image ,const std::string&  image_mod)
{
	builder_frame_parameters::image = image;
	builder_frame_parameters::image_mod = image_mod;
	return *this;
}

frame_builder& frame_builder::image_diagonal(const std::string& image_diagonal,const std::string& image_mod)
{
	builder_frame_parameters::image_diagonal = image_diagonal;
	builder_frame_parameters::image_mod = image_mod;
	return *this;
}

frame_builder& frame_builder::sound(const std::string& sound)
{
	builder_frame_parameters::sound = sound;
	return *this;
}

frame_builder& frame_builder::text(const std::string& text,const color_t text_color)
{
	builder_frame_parameters::text = text;
	builder_frame_parameters::text_color = text_color;
	return *this;
}

frame_builder& frame_builder::halo(const std::string& halo, const std::string& halo_x, const std::string& halo_y,const std::string&  halo_mod)
{
	builder_frame_parameters::halo = halo;
	builder_frame_parameters::halo_x = halo_x;
	builder_frame_parameters::halo_y = halo_y;
	builder_frame_parameters::halo_mod = halo_mod;
	return *this;
}

frame_builder& frame_builder::duration(const int duration)
{
	builder_frame_parameters::duration = duration;
	return *this;
}

frame_builder& frame_builder::blend(const std::string& blend_ratio,const color_t blend_color)
{
	builder_frame_parameters::blend_with = blend_color;
	builder_frame_parameters::blend_ratio = blend_ratio;
	return *this;
}

frame_builder& frame_builder::highlight(const std::string& highlight)
{
	builder_frame_parameters::highlight_ratio = highlight;
	return *this;
}

frame_builder& frame_builder::offset(const std::string& offset)
{
	builder_frame_parameters::offset = offset;
	return *this;
}

frame_builder& frame_builder::submerge(const std::string& submerge)
{
	builder_frame_parameters::submerge = submerge;
	return *this;
}

frame_builder& frame_builder::x(const std::string& x)
{
	builder_frame_parameters::x = x;
	return *this;
}

frame_builder& frame_builder::y(const std::string& y)
{
	builder_frame_parameters::y = y;
	return *this;
}

frame_builder& frame_builder::directional_x(const std::string& directional_x)
{
	builder_frame_parameters::directional_x = directional_x;
	return *this;
}

frame_builder& frame_builder::directional_y(const std::string& directional_y)
{
	builder_frame_parameters::directional_y = directional_y;
	return *this;
}

frame_builder& frame_builder::auto_vflip(const bool auto_vflip)
{
	builder_frame_parameters::auto_vflip = auto_vflip;
	return *this;
}

frame_builder& frame_builder::auto_hflip(const bool auto_hflip)
{
	builder_frame_parameters::auto_hflip = auto_hflip;
	return *this;
}

frame_builder& frame_builder::primary_frame(const bool primary_frame)
{
	builder_frame_parameters::primary_frame = primary_frame;
	return *this;
}

frame_builder& frame_builder::drawing_layer(const std::string& drawing_layer)
{
	builder_frame_parameters::drawing_layer = drawing_layer;
	return *this;
}

template<> template<>
parsed_frame_parameters::basic_frame_parameters(const builder_frame_parameters& builder, int duration)
	: duration(duration ? duration : builder.duration)
	, image(builder.image, duration)
	, image_diagonal(builder.image_diagonal, duration)
	, image_mod(builder.image_mod)
	, halo(builder.halo, duration)
	, halo_x(builder.halo_x, duration)
	, halo_y(builder.halo_y, duration)
	, halo_mod(builder.halo_mod)
	, sound(builder.sound)
	, text(builder.text)
	, text_color(builder.text_color)
	, blend_with(builder.blend_with)
	, blend_ratio(builder.blend_ratio, duration)
	, highlight_ratio(builder.highlight_ratio, duration)
	, offset(builder.offset, duration)
	, submerge(builder.submerge, duration)
	, x(builder.x, duration)
	, y(builder.y, duration)
	, directional_x(builder.directional_x, duration)
	, directional_y(builder.directional_y, duration)
	, auto_vflip(builder.auto_vflip)
	, auto_hflip(builder.auto_hflip)
	, primary_frame(builder.primary_frame)
	, drawing_layer(builder.drawing_layer, duration)
{}

frame_parsed_parameters::frame_parsed_parameters(const frame_builder& builder, int duration)
	: parsed_frame_parameters(builder, duration)
{}

int frame_parsed_parameters::duration() const
{
	return parsed_frame_parameters::duration;
}

bool frame_parsed_parameters::does_not_change() const
{
	return
		image.does_not_change() &&
		image_diagonal.does_not_change() &&
		halo.does_not_change() &&
		halo_x.does_not_change() &&
		halo_y.does_not_change() &&
		blend_ratio.does_not_change() &&
		highlight_ratio.does_not_change() &&
		offset.does_not_change() &&
		submerge.does_not_change() &&
		x.does_not_change() &&
		y.does_not_change() &&
		directional_x.does_not_change() &&
		directional_y.does_not_change() &&
		drawing_layer.does_not_change();
}

bool frame_parsed_parameters::need_update() const
{
	return !this->does_not_change();
}

const frame_parameters frame_parsed_parameters::parameters(int current_time) const
{
	frame_parameters result;
	result.duration = parsed_frame_parameters::duration;
	result.image = image.get_current_element(current_time);
	result.image_diagonal = image_diagonal.get_current_element(current_time);
	result.image_mod = image_mod;
	result.halo = halo.get_current_element(current_time);
	result.halo_x = halo_x.get_current_element(current_time);
	result.halo_y = halo_y.get_current_element(current_time);
	result.halo_mod = halo_mod;
	result.sound = sound;
	result.text = text;
	result.text_color = text_color;
	result.blend_with = blend_with;
	result.blend_ratio = blend_ratio.get_current_element(current_time);
	result.highlight_ratio = highlight_ratio.get_current_element(current_time,1.0);
	result.offset = offset.get_current_element(current_time,-1000);
	result.submerge = submerge.get_current_element(current_time);
	result.x = x.get_current_element(current_time);
	result.y = y.get_current_element(current_time);
	result.directional_x = directional_x.get_current_element(current_time);
	result.directional_y = directional_y.get_current_element(current_time);
	result.auto_vflip = auto_vflip;
	result.auto_hflip = auto_hflip;
	result.primary_frame = primary_frame;
	result.drawing_layer = drawing_layer.get_current_element(current_time,display::LAYER_UNIT_DEFAULT-display::LAYER_UNIT_FIRST);
	return result;
}

void frame_parsed_parameters::override(int new_duration,
		const std::string& new_highlight,
		const std::string& new_blend_ratio,
		color_t new_blend_color,
		const std::string& new_offset,
		const std::string& new_layer,
		const std::string& new_modifiers)
{
	if(!new_highlight.empty()) {
		highlight_ratio = progressive_double(new_highlight, new_duration);
	} else if(new_duration != parsed_frame_parameters::duration) {
		highlight_ratio = progressive_double(highlight_ratio.get_original(), new_duration);
	}

	if(!new_offset.empty()) {
		offset = progressive_double(new_offset, new_duration);
	} else if(new_duration != parsed_frame_parameters::duration) {
		offset = progressive_double(offset.get_original(), new_duration);
	}

	if(!new_blend_ratio.empty()) {
		blend_ratio = progressive_double(new_blend_ratio, new_duration);
		blend_with = new_blend_color;
	} else if(new_duration != parsed_frame_parameters::duration) {
		blend_ratio = progressive_double(blend_ratio.get_original(), new_duration);
	}

	if(!new_layer.empty()) {
		drawing_layer = progressive_int(new_layer, new_duration);
	} else if(new_duration != parsed_frame_parameters::duration) {
		drawing_layer = progressive_int(drawing_layer.get_original(), new_duration);
	}

	if(!new_modifiers.empty()) {
		image_mod += new_modifiers;
	}

	if(new_duration != parsed_frame_parameters::duration) {
		image = progressive_image(image.get_original(), new_duration);
		image_diagonal = progressive_image(image_diagonal.get_original(), new_duration);
		halo = progressive_string(halo.get_original(), new_duration);
		halo_x = progressive_int(halo_x.get_original(), new_duration);
		halo_y = progressive_int(halo_y.get_original(), new_duration);
		submerge = progressive_double(submerge.get_original(), new_duration);
		x = progressive_int(x.get_original(), new_duration);
		y = progressive_int(y.get_original(), new_duration);
		directional_x = progressive_int(directional_x.get_original(), new_duration);
		directional_y = progressive_int(directional_y.get_original(), new_duration);
		parsed_frame_parameters::duration = new_duration;
	}
}

std::vector<std::string> frame_parsed_parameters::debug_strings() const
{
	std::vector<std::string> v;

	if(parsed_frame_parameters::duration > 0) {
		v.push_back("duration=" + utils::half_signed_value(parsed_frame_parameters::duration));
	}

	if(!image.get_original().empty()) {
		v.push_back("image=" + image.get_original());
	}

	if(!image_diagonal.get_original().empty()) {
		v.push_back("image_diagonal=" + image_diagonal.get_original());
	}

	if(!image_mod.empty()) {
		v.push_back("image_mod=" + image_mod);
	}

	if(!halo.get_original().empty()) {
		v.push_back("halo=" + halo.get_original());
	}

	if(!halo_x.get_original().empty()) {
		v.push_back("halo_x=" + halo_x.get_original());
	}

	if(!halo_y.get_original().empty()) {
		v.push_back("halo_y=" + halo_y.get_original());
	}

	if(!halo_mod.empty()) {
		v.push_back("halo_mod=" + halo_mod);
	}

	if(!sound.empty()) {
		v.push_back("sound=" + sound);
	}

	if(!text.empty()) {
		v.push_back("text=" + text);

		if(text_color) {
			v.push_back("text_color=" + text_color->to_rgba_string());
		}
	}

	if(!blend_ratio.get_original().empty()) {
		v.push_back("blend_ratio=" + blend_ratio.get_original());

		if(blend_with) {
			v.push_back("blend_with=" + blend_with->to_rgba_string());
		}
	}

	if(!highlight_ratio.get_original().empty()) {
		v.push_back("highlight_ratio=" + highlight_ratio.get_original());
	}

	if(!offset.get_original().empty()) {
		v.push_back("offset=" + offset.get_original());
	}

	if(!submerge.get_original().empty()) {
		v.push_back("submerge=" + submerge.get_original());
	}

	if(!x.get_original().empty()) {
		v.push_back("x=" + x.get_original());
	}

	if(!y.get_original().empty()) {
		v.push_back("y=" + y.get_original());
	}

	if(!directional_x.get_original().empty()) {
		v.push_back("directional_x=" + directional_x.get_original());
	}

	if(!directional_y.get_original().empty()) {
		v.push_back("directional_y=" + directional_y.get_original());
	}

	if(!boost::indeterminate(auto_vflip)) {
		v.push_back("auto_vflip=" + std::string(auto_vflip ? "true" : "false"));
	}

	if(!boost::indeterminate(auto_hflip)) {
		v.push_back("auto_hflip=" + std::string(auto_hflip ? "true" : "false"));
	}

	if(!boost::indeterminate(primary_frame)) {
		v.push_back("primary_frame=" + std::string(primary_frame ? "true" : "false"));
	}

	if(!drawing_layer.get_original().empty()) {
		v.push_back("drawing_layer=" + drawing_layer.get_original());
	}

	return v;
}

void unit_frame::redraw(const int frame_time, bool on_start_time, bool in_scope_of_frame,
		const map_location& src, const map_location& dst,
		halo::handle& halo_id, halo::manager& halo_man,
		const frame_parameters& animation_val, const frame_parameters& engine_val) const
{
	game_display* game_disp = game_display::get_singleton();

	const int xsrc = game_disp->get_location_x(src);
	const int ysrc = game_disp->get_location_y(src);
	const int xdst = game_disp->get_location_x(dst);
	const int ydst = game_disp->get_location_y(dst);
	const map_location::DIRECTION direction = src.get_relative_dir(dst);

	const frame_parameters current_data = merge_parameters(frame_time,animation_val,engine_val);
	double tmp_offset = current_data.offset;

	// Debug code to see the number of frames and their position
	//if(tmp_offset) {
	//	std::cout << static_cast<int>(tmp_offset * 100) << "," << "\n";
	//}

	if(on_start_time) {
		// Stuff that should be done only once per frame
		if(!current_data.sound.empty()  ) {
			sound::play_sound(current_data.sound);
		}

		if(!current_data.text.empty() && current_data.text_color) {
			game_disp->float_label(src, current_data.text, *current_data.text_color);
		}
	}

	image::locator image_loc;
	if(direction != map_location::NORTH && direction != map_location::SOUTH) {
		image_loc = image::locator(current_data.image_diagonal, current_data.image_mod);
	}

	if(image_loc.is_void() || image_loc.get_filename().empty()) { // invalid diag image, or not diagonal
		image_loc = image::locator(current_data.image, current_data.image_mod);
	}

	surface image;
	if(!image_loc.is_void() && !image_loc.get_filename().empty()) { // invalid diag image, or not diagonal
		image=image::get_image(image_loc, image::SCALED_TO_ZOOM);
	}

	const int d2 = display::get_singleton()->hex_size() / 2;

	const int x = static_cast<int>(tmp_offset * xdst + (1.0 - tmp_offset) * xsrc) + d2;
	const int y = static_cast<int>(tmp_offset * ydst + (1.0 - tmp_offset) * ysrc) + d2;

	if(image != nullptr) {
		bool facing_west = (
			direction == map_location::NORTH_WEST ||
			direction == map_location::SOUTH_WEST);

		bool facing_north = (
			direction == map_location::NORTH_WEST ||
			direction == map_location::NORTH ||
			direction == map_location::NORTH_EAST);

		if(!current_data.auto_hflip) { facing_west = false; }
		if(!current_data.auto_vflip) { facing_north = true; }

		int my_x = x + current_data.x - image->w / 2;
		int my_y = y + current_data.y - image->h / 2;

		if(facing_west) {
			my_x -= current_data.directional_x;
		} else {
			my_x += current_data.directional_x;
		}

		if(facing_north) {
			my_y += current_data.directional_y;
		} else {
			my_y -= current_data.directional_y;
		}

		game_display::get_singleton()->render_image(my_x, my_y,
			static_cast<display::drawing_layer>(display::LAYER_UNIT_FIRST + current_data.drawing_layer),
			src, image, facing_west, false,
			ftofxp(current_data.highlight_ratio), current_data.blend_with ? *current_data.blend_with : color_t(),
			current_data.blend_ratio, current_data.submerge, !facing_north);
	}

	halo_id = halo::handle(); //halo::NO_HALO;

	if(!in_scope_of_frame) { //check after frame as first/last frame image used in defense/attack anims
		return;
	}

	// No halos, exit
	if(current_data.halo.empty()) {
		return;
	}

	halo::ORIENTATION orientation;
	switch(direction)
	{
		case map_location::NORTH:
		case map_location::NORTH_EAST:
			orientation = halo::NORMAL;
			break;
		case map_location::SOUTH_EAST:
		case map_location::SOUTH:
			if(!current_data.auto_vflip) {
				orientation = halo::NORMAL;
			} else {
				orientation = halo::VREVERSE;
			}
			break;
		case map_location::SOUTH_WEST:
			if(!current_data.auto_vflip) {
				orientation = halo::HREVERSE;
			} else {
				orientation = halo::HVREVERSE;
			}
			break;
		case map_location::NORTH_WEST:
			orientation = halo::HREVERSE;
			break;
		case map_location::NDIRECTIONS:
		default:
			orientation = halo::NORMAL;
			break;
	}

	if(direction != map_location::SOUTH_WEST && direction != map_location::NORTH_WEST) {
		halo_id = halo_man.add(
			static_cast<int>(x + current_data.halo_x * game_display::get_singleton()->get_zoom_factor()),
			static_cast<int>(y + current_data.halo_y * game_display::get_singleton()->get_zoom_factor()),
			current_data.halo  + current_data.halo_mod,
			map_location(-1, -1),
			orientation
		);
	} else {
		halo_id = halo_man.add(
			static_cast<int>(x - current_data.halo_x * game_display::get_singleton()->get_zoom_factor()),
			static_cast<int>(y + current_data.halo_y * game_display::get_singleton()->get_zoom_factor()),
			current_data.halo  + current_data.halo_mod,
			map_location(-1, -1),
			orientation
		);
	}
}

std::set<map_location> unit_frame::get_overlaped_hex(const int frame_time, const map_location& src, const map_location& dst,
		const frame_parameters& animation_val, const frame_parameters& engine_val) const
{
	display* disp = display::get_singleton();

	const int xsrc = disp->get_location_x(src);
	const int ysrc = disp->get_location_y(src);
	const int xdst = disp->get_location_x(dst);
	const int ydst = disp->get_location_y(dst);
	const map_location::DIRECTION direction = src.get_relative_dir(dst);

	const frame_parameters current_data = merge_parameters(frame_time, animation_val, engine_val);

	double tmp_offset = current_data.offset;
	const int d2 = game_display::get_singleton()->hex_size() / 2;

	image::locator image_loc;
	if(direction != map_location::NORTH && direction != map_location::SOUTH) {
		image_loc = image::locator(current_data.image_diagonal, current_data.image_mod);
	}

	if(image_loc.is_void() || image_loc.get_filename().empty()) { // invalid diag image, or not diagonal
		image_loc = image::locator(current_data.image, current_data.image_mod);
	}

	// We always invalidate our own hex because we need to be called at redraw time even
	// if we don't draw anything in the hex itself
	std::set<map_location> result;
	if(tmp_offset == 0 && current_data.x == 0 && current_data.directional_x == 0 && image::is_in_hex(image_loc)) {
		result.insert(src);

		bool facing_north = (
			direction == map_location::NORTH_WEST ||
			direction == map_location::NORTH ||
			direction == map_location::NORTH_EAST);

		if(!current_data.auto_vflip) { facing_north = true; }

		int my_y = current_data.y;
		if(facing_north) {
			my_y += current_data.directional_y;
		} else {
			my_y -= current_data.directional_y;
		}

		if(my_y < 0) {
			result.insert(src.get_direction(map_location::NORTH));
			result.insert(src.get_direction(map_location::NORTH_EAST));
			result.insert(src.get_direction(map_location::NORTH_WEST));
		} else if(my_y > 0) {
			result.insert(src.get_direction(map_location::SOUTH));
			result.insert(src.get_direction(map_location::SOUTH_EAST));
			result.insert(src.get_direction(map_location::SOUTH_WEST));
		}
	} else {
		int w = 0, h = 0;

#ifdef _OPENMP
#pragma omp critical(frame_surface) // with the way surfaces work it's hard to lock the refcount within sdl_utils
#endif //_OPENMP

		{
			surface image;
			if(!image_loc.is_void() && !image_loc.get_filename().empty()) { // invalid diag image, or not diagonal
				image = image::get_image(image_loc, image::SCALED_TO_ZOOM);
			}

			if(image != nullptr) {
				w = image->w;
				h = image->h;
			}
		}

		if(w != 0 || h != 0) {
			const int x = static_cast<int>(tmp_offset * xdst + (1.0 - tmp_offset) * xsrc);
			const int y = static_cast<int>(tmp_offset * ydst + (1.0 - tmp_offset) * ysrc);

			bool facing_west = (
				direction == map_location::NORTH_WEST ||
				direction == map_location::SOUTH_WEST);

			bool facing_north = (
				direction == map_location::NORTH_WEST ||
				direction == map_location::NORTH ||
				direction == map_location::NORTH_EAST);

			if(!current_data.auto_vflip) { facing_north = true; }
			if(!current_data.auto_hflip) { facing_west = false; }

			int my_x = x + current_data.x + d2 - w / 2;
			int my_y = y + current_data.y + d2 - h / 2;

			if(facing_west) {
				my_x += current_data.directional_x;
			} else {
				my_x -= current_data.directional_x;
			}

			if(facing_north) {
				my_y += current_data.directional_y;
			} else {
				my_y -= current_data.directional_y;
			}

			// Check if our underlying hexes are invalidated. If we need to update ourselves because we changed,
			// invalidate our hexes and return whether or not was sucessful.
			const SDL_Rect r = {my_x, my_y, w, h};
			display::rect_of_hexes underlying_hex = disp->hexes_under_rect(r);

			result.insert(src);
			result.insert(underlying_hex.begin(), underlying_hex.end());
		} else {
			// We have no "redraw surface" but we still need to invalidate our own hex in case we have a halo
			// and/or sound that needs a redraw.
			result.insert(src);
			result.insert(dst);
		}
	}

	return result;
}

/**
 * This function merges the value provided by:
 *  - the frame
 *  - the engine (poison, flying unit...)
 *  - the animation as a whole
 *
 * There is no absolute rule for merging, so creativity is the rule. If a value is never provided by the engine, assert.
 * This way if it becomes used, people will easily find the right place to look.
 */
const frame_parameters unit_frame::merge_parameters(int current_time, const frame_parameters& animation_val,
		const frame_parameters& engine_val) const
{
	frame_parameters result;
	const frame_parameters& current_val = builder_.parameters(current_time);

	result.primary_frame = engine_val.primary_frame;
	if(!boost::logic::indeterminate(animation_val.primary_frame)) {
		result.primary_frame = animation_val.primary_frame;
	}

	if(!boost::logic::indeterminate(current_val.primary_frame)) {
		result.primary_frame = current_val.primary_frame;
	}

	// Convert the tribool to bool
	const bool primary = result.primary_frame == true || boost::logic::indeterminate(result.primary_frame);

	/** The engine provides a default image to use for the unit when none is available */
	result.image = current_val.image.is_void() || current_val.image.get_filename().empty()
		? animation_val.image
		: current_val.image;

	if(primary && (result.image.is_void() || result.image.get_filename().empty())) {
		result.image = engine_val.image;
	}

	/** The engine provides a default image to use for the unit when none is available */
	result.image_diagonal = current_val.image_diagonal.is_void() || current_val.image_diagonal.get_filename().empty()
		? animation_val.image_diagonal
		: current_val.image_diagonal;

	if(primary && (result.image_diagonal.is_void() || result.image_diagonal.get_filename().empty())) {
		result.image_diagonal = engine_val.image_diagonal;
	}

	/**
	 * The engine provides a string for "petrified" and "team color" modifications.
     * Note that image_mod is the complete modification and halo_mod is only the TC part.
	 */
	result.image_mod = current_val.image_mod + animation_val.image_mod;
	if(primary) {
		result.image_mod += engine_val.image_mod;
	} else {
		result.image_mod += engine_val.halo_mod;
	}

	assert(engine_val.halo.empty());
	result.halo = current_val.halo.empty() ? animation_val.halo : current_val.halo;

	assert(engine_val.halo_x == 0);
	result.halo_x = current_val.halo_x ? current_val.halo_x : animation_val.halo_x;

	/** The engine provides a y modification for terrain with height adjust and flying units */
	result.halo_y = current_val.halo_y ? current_val.halo_y : animation_val.halo_y;
	result.halo_y += engine_val.halo_y;

	result.halo_mod = current_val.halo_mod + animation_val.halo_mod;
	result.halo_mod += engine_val.halo_mod;

	assert(engine_val.duration == 0);
	result.duration = current_val.duration;

	assert(engine_val.sound.empty());
	result.sound = current_val.sound.empty() ? animation_val.sound : current_val.sound;

	assert(engine_val.text.empty());
	result.text = current_val.text.empty() ? animation_val.text : current_val.text;

	assert(!engine_val.text_color);
	result.text_color = current_val.text_color ? current_val.text_color : animation_val.text_color;

	/** The engine provides a blend color for poisoned units */
	result.blend_with = current_val.blend_with ? current_val.blend_with : animation_val.blend_with;
	if(primary && engine_val.blend_with) {
		result.blend_with = engine_val.blend_with->blend_lighten(result.blend_with ? *result.blend_with : color_t(0,0,0));
	}

	/** The engine provides a blend color for poisoned units */
	result.blend_ratio = current_val.blend_ratio?current_val.blend_ratio:animation_val.blend_ratio;
	if(primary && engine_val.blend_ratio) {
		result.blend_ratio = std::min(result.blend_ratio + engine_val.blend_ratio, 1.0);
	}

	/** The engine provides a highlight ratio for selected units and visible "invisible" units */
	result.highlight_ratio = (current_val.highlight_ratio < 0.999 || current_val.highlight_ratio > 1.001) ?
		current_val.highlight_ratio : animation_val.highlight_ratio;
	if(primary && (engine_val.highlight_ratio < 0.999 || engine_val.highlight_ratio > 1.001)) {
		result.highlight_ratio = result.highlight_ratio * engine_val.highlight_ratio; // selected unit
	}

	assert(engine_val.offset == 0);
	result.offset = (current_val.offset != -1000) ? current_val.offset : animation_val.offset;
	if(result.offset == -1000) {
		result.offset = 0.0;
	}

	/** The engine provides a submerge for units in water */
	result.submerge = current_val.submerge ? current_val.submerge : animation_val.submerge;
	if(primary && engine_val.submerge && !result.submerge) {
		result.submerge = engine_val.submerge;
	}

	assert(engine_val.x == 0);
	result.x = current_val.x ? current_val.x : animation_val.x;

	/** The engine provides a y modification for terrain with height adjust and flying units */
	result.y = current_val.y?current_val.y:animation_val.y;
	result.y += engine_val.y;

	assert(engine_val.directional_x == 0);
	result.directional_x = current_val.directional_x ? current_val.directional_x : animation_val.directional_x;

	assert(engine_val.directional_y == 0);
	result.directional_y = current_val.directional_y ? current_val.directional_y : animation_val.directional_y;

	assert(engine_val.drawing_layer == display::LAYER_UNIT_DEFAULT - display::LAYER_UNIT_FIRST);
	result.drawing_layer = current_val.drawing_layer != display::LAYER_UNIT_DEFAULT-display::LAYER_UNIT_FIRST
		? current_val.drawing_layer
		: animation_val.drawing_layer;

	/** The engine provides us with a default value to compare to. Update if different */
	result.auto_hflip = engine_val.auto_hflip;

	if(!boost::logic::indeterminate(animation_val.auto_hflip)) {
		result.auto_hflip = animation_val.auto_hflip;
	}

	if(!boost::logic::indeterminate(current_val.auto_hflip)) {
		result.auto_hflip = current_val.auto_hflip;
	}

	if(boost::logic::indeterminate(result.auto_hflip)) {
		result.auto_hflip = true;
	}

	result.auto_vflip = engine_val.auto_vflip;

	if(!boost::logic::indeterminate(animation_val.auto_vflip)) {
		result.auto_vflip = animation_val.auto_vflip;
	}

	if(!boost::logic::indeterminate(current_val.auto_vflip)) {
		result.auto_vflip = current_val.auto_vflip;
	}

	if(boost::logic::indeterminate(result.auto_vflip)) {
		result.auto_vflip = !primary;
	}

	return result;
}
