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

/**
 *  @file
 *  Frame for unit's animation sequence.
 */

#ifndef UNIT_FRAME_H_INCLUDED
#define UNIT_FRAME_H_INCLUDED

#include "units/frame_private.hpp"

#include "halo.hpp"
#include "image.hpp"

#include <boost/logic/tribool.hpp>
#include <boost/optional.hpp>

class config;

template<typename ProgInt, typename ProgReal, typename ProgStr, typename Image>
struct basic_frame_parameters {
	basic_frame_parameters();
	template<typename A, typename B, typename C, typename D>
	basic_frame_parameters(const basic_frame_parameters<A,B,C,D>&, int);

	int duration;

	Image image, image_diagonal;

	std::string image_mod;

	ProgStr halo;
	ProgInt halo_x, halo_y;
	std::string halo_mod;

	std::string sound, text;
	boost::optional<color_t> text_color;

	boost::optional<color_t> blend_with;
	ProgReal blend_ratio;

	ProgReal highlight_ratio;
	ProgReal offset, submerge;

	ProgInt x, y, directional_x, directional_y;

	boost::tribool auto_vflip, auto_hflip;
	boost::tribool primary_frame;

	ProgInt drawing_layer;
};

/** All parameters from a frame at a given instant */
using frame_parameters = basic_frame_parameters<int, double, std::string, image::locator>;
using builder_frame_parameters = basic_frame_parameters<std::string, std::string, std::string, std::string>;
using parsed_frame_parameters = basic_frame_parameters<progressive_int, progressive_double, progressive_string, progressive_image>;

/**
 * Easily build frame parameters with the serialized constructors
 */
class frame_parsed_parameters;
class frame_builder : private builder_frame_parameters
{
public:
	frame_builder();
	frame_builder(const config& cfg, const std::string& frame_string = "");

	/** Allow easy chained modifications. Will raised assert if used after initialization */
	frame_builder& duration(const int duration);
	frame_builder& image(const std::string& image, const std::string& image_mod = "");
	frame_builder& image_diagonal(const std::string& image_diagonal, const std::string& image_mod = "");
	frame_builder& sound(const std::string& sound);
	frame_builder& text(const std::string& text, const color_t text_color);
	frame_builder& halo(const std::string& halo, const std::string& halo_x, const std::string& halo_y, const std::string& halo_mod);
	frame_builder& blend(const std::string& blend_ratio, const color_t blend_color);
	frame_builder& highlight(const std::string& highlight);
	frame_builder& offset(const std::string& offset);
	frame_builder& submerge(const std::string& submerge);
	frame_builder& x(const std::string& x);
	frame_builder& y(const std::string& y);
	frame_builder& directional_x(const std::string& directional_x);
	frame_builder& directional_y(const std::string& directional_y);
	frame_builder& auto_vflip(const bool auto_vflip);
	frame_builder& auto_hflip(const bool auto_hflip);
	frame_builder& primary_frame(const bool primary_frame);
	frame_builder& drawing_layer(const std::string& drawing_layer);

private:
	friend class frame_parsed_parameters;
};

/**
 * Keep most parameters in a separate class to simplify the handling of the large
 * number of parameters between the frame level and animation level.
 */
class frame_parsed_parameters : private parsed_frame_parameters
{
public:
	frame_parsed_parameters(const frame_builder& builder = frame_builder(), int override_duration = 0);

	void override(int duration,
		const std::string& highlight = "",
		const std::string& blend_ratio = "",
		color_t blend_color = {0,0,0},
		const std::string& offset = "",
		const std::string& layer = "",
		const std::string& modifiers = "");

	/** Getters for the different parameters */
	const frame_parameters parameters(int current_time) const;

	int duration() const;
	bool does_not_change() const;
	bool need_update() const;

	/** Contents of frame in strings */
	std::vector<std::string> debug_strings() const;
};

/** Describes a unit's animation sequence. */
class unit_frame
{
public:
	// Constructors
	unit_frame(const frame_builder& builder = frame_builder()) : builder_(builder) {}

	void redraw(const int frame_time, bool on_start_time, bool in_scope_of_frame, const map_location& src, const map_location& dst,
		halo::handle& halo_id, halo::manager& halo_man, const frame_parameters& animation_val, const frame_parameters& engine_val) const;

	const frame_parameters merge_parameters(int current_time, const frame_parameters& animation_val,
		const frame_parameters& engine_val = frame_parameters()) const;

	const frame_parameters parameters(int current_time) const
	{
		return builder_.parameters(current_time);
	}

	const frame_parameters end_parameters() const
	{
		return builder_.parameters(duration());
	}

	int duration() const
	{
		return builder_.duration();
	}

	bool does_not_change() const
	{
		return builder_.does_not_change();
	}

	bool need_update() const
	{
		return builder_.need_update();
	}

	std::vector<std::string> debug_strings() const
	{
		// Contents of frame in strings
		return builder_.debug_strings();
	}

	std::set<map_location> get_overlaped_hex(const int frame_time, const map_location& src, const map_location& dst,
		const frame_parameters& animation_val, const frame_parameters& engine_val) const;

private:
	frame_parsed_parameters builder_;
};

#endif
