/*
Copyright (C) 2017 by the Battle for Wesnoth Project http://www.wesnoth.org/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.

See the COPYING file for more details.
*/

#include "lua_audio.hpp"

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "scripting/lua_common.hpp"
#include "sound.hpp"
#include "sound_music_track.hpp"
#include "config_assign.hpp"
#include "preferences.hpp"

static const char* Track = "music track";

class music_track {
	int i;
	sound::music_track& track;
public:
	explicit music_track(int i) : i(i), track(sound::get_track(i)) {}
	bool valid() {
		sound::music_track& current = sound::get_track(i);
		return &track == &current && track.valid() && track.id() == current.id();
	}
	sound::music_track& operator*() {
		return track;
	}
	sound::music_track* operator->() {
		return &track;
	}
	int index() {
		return i;
	}
};

static music_track* push_track(lua_State* L, int i) {
	music_track* trk = new(L) music_track(i);
	luaL_setmetatable(L, Track);
	return trk;
}

static music_track* get_track(lua_State* L, int i) {
	return static_cast<music_track*>(luaL_checkudata(L, i, Track));
}

static int impl_music_get(lua_State* L) {
	if(lua_isnumber(L, 2)) {
		push_track(L, lua_tointeger(L, 2) - 1);
		return 1;
	}
	const char* m = luaL_checkstring(L, 2);
	if(strcmp(m, "current") == 0) {
		push_track(L, sound::get_current_track());
		return 1;
	}
	return_int_attrib("current_i", sound::get_current_track() + 1);
	// This calculation reverses the one used in [volume] to get back the relative volume level.
	// (Which is the same calculation that's duplicated in impl_music_set.)
	return_float_attrib("volume", sound::get_music_volume() * 100.0f / preferences::music_volume());
	return luaW_getmetafield(L, 1, m);
}

static int impl_music_set(lua_State* L) {
	if(lua_isnumber(L, 2)) {
		music_track& track = *get_track(L, 3);
		sound::set_track(lua_tointeger(L, 2), *track);
		return 0;
	}
	const char* m = luaL_checkstring(L, 2);
	modify_float_attrib_check_range("volume", sound::set_music_volume(value * preferences::music_volume() / 100.0f), 0.0, 100.0)
	// TODO: Set "current" and "current_i"
	return 0;
}

static int impl_music_len(lua_State* L) {
	lua_pushinteger(L, sound::get_num_tracks());
	return 1;
}

static int intf_music_play(lua_State* L) {
	sound::play_music_once(luaL_checkstring(L, 1));
	return 0;
}

static int intf_music_add(lua_State* L) {
	config cfg = config_of
		("name", luaL_checkstring(L, 1))
		("append", true);
	bool found_ms_before = false, found_ms_after = false, found_imm = false;
	for(int i = 2; i <= lua_gettop(L); i++) {
		if(lua_isboolean(L, i)) {
			if(found_imm) {
				return luaL_argerror(L, i, "only one boolean argument may be passed");
			} else {
				cfg["immediate"] = luaW_toboolean(L, i);
			}
		} else if(lua_isnumber(L, i)) {
			if(found_ms_after) {
				return luaL_argerror(L, i, "only two integer arguments may be passed");
			} else if(found_ms_before) {
				cfg["ms_after"] = lua_tointeger(L, i);
				found_ms_after = true;
			} else {
				cfg["ms_before"] = lua_tointeger(L, i);
				found_ms_before = true;
			}
		} else {
			return luaL_argerror(L, i, "unrecognized argument");
		}
	}
	sound::play_music_config(cfg);
	return 0;
}

static int intf_music_clear(lua_State*) {
	sound::empty_playlist();
	return 0;
}

static int intf_music_commit(lua_State*) {
	sound::commit_music_changes();
	return 0;
}

static int impl_track_get(lua_State* L) {
	music_track& track = *get_track(L, 1);
	const char* m = luaL_checkstring(L, 2);
	return_bool_attrib("valid", track.valid());
	if(!track.valid()) {
		return luaL_error(L, "Tried to access member of track that is no longer valid.");
	}
	return_bool_attrib("append", track->append());
	return_bool_attrib("shuffle", track->shuffle());
	return_bool_attrib("immediate", track->immediate());
	return_bool_attrib("once", track->play_once());
	return_int_attrib("ms_before", track->ms_before());
	return_int_attrib("ms_after", track->ms_after());
	return_string_attrib("name", track->id());
	return_string_attrib("title", track->title());
	return luaW_getmetafield(L, 1, m);
}

static int impl_track_set(lua_State* L) {
	music_track& track = *get_track(L, 1);
	const char* m = luaL_checkstring(L, 2);
	modify_bool_attrib("shuffle", track->set_shuffle(value));
	modify_bool_attrib("once", track->set_play_once(value));
	modify_int_attrib("ms_before", track->set_ms_before(value));
	modify_int_attrib("ms_after", track->set_ms_after(value));
	return 0;
}

static int impl_track_eq(lua_State* L) {
	music_track* a = get_track(L, 1);
	music_track* b = get_track(L, 2);
	if(!a || !b) {
		// This implies that one argument is not a music track, though I suspect this is dead code...?
		// Does Lua ever call this if the arguments are not of the same type?
		lua_pushboolean(L, false);
		return 1;
	}
	if(!a->valid() && !b->valid()) {
		lua_pushboolean(L, true);
		return 1;
	}
	if(a->valid() && b->valid()) {
		music_track& lhs = *a;
		music_track& rhs = *b;
		lua_pushboolean(L, lhs->id() == rhs->id() && lhs->shuffle() == rhs->shuffle() && lhs->play_once() == rhs->play_once() && lhs->ms_before() == rhs->ms_before() && lhs->ms_after() == rhs->ms_after());
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
}

namespace lua_audio {
	std::string register_table(lua_State* L) {
		// The music playlist metatable
		lua_getglobal(L, "wesnoth");
		lua_newuserdata(L, 0);
		lua_createtable(L, 0, 4);
		static luaL_Reg pl_callbacks[] = {
			{ "__index", impl_music_get },
			{ "__newindex", impl_music_set },
			{ "__len", impl_music_len },
			{ "play", intf_music_play },
			{ "add", intf_music_add },
			{ "clear", intf_music_clear },
			{ "commit", intf_music_commit },
			{ nullptr, nullptr },
		};
		luaL_setfuncs(L, pl_callbacks, 0);
		lua_pushstring(L, "music playlist");
		lua_setfield(L, -2, "__metatable");
		lua_setmetatable(L, -2);
		lua_setfield(L, -2, "playlist");
		lua_pop(L, 1);

		// The music track metatable
		luaL_newmetatable(L, Track);
		static luaL_Reg track_callbacks[] = {
			{ "__index", impl_track_get },
			{ "__newindex", impl_track_set },
			{ "__eq", impl_track_eq },
			{ nullptr, nullptr },
		};
		luaL_setfuncs(L, track_callbacks, 0);
		lua_pushstring(L, Track);
		lua_setfield(L, -2, "__metatable");

		return "Adding music playlist table...";
	}
}
