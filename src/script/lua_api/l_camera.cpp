// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "l_camera.h"
#include "script/common/c_converter.h"
#include "l_internal.h"
#include "client/content_cao.h"
#include "client/camera.h"
#include "client/client.h"
#include "client/localplayer.h"
#include <ICameraSceneNode.h>
#include "client/clientevent.h"
#include "client/clientmap.h"
#include "skyparams.h"

LuaCamera::LuaCamera(Camera *m) : m_camera(m)
{
}

void LuaCamera::create(lua_State *L, Camera *m)
{
	lua_getglobal(L, "core");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	lua_getfield(L, -1, "camera");

	// Duplication check
	if (lua_type(L, -1) == LUA_TUSERDATA) {
		lua_pop(L, 1);
		return;
	}

	LuaCamera *o = new LuaCamera(m);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);

	lua_pushvalue(L, lua_gettop(L));
	lua_setfield(L, objectstable, "camera");
}

// set_camera_mode(self, mode)
int LuaCamera::l_set_camera_mode(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	GenericCAO *playercao = getClient(L)->getEnv().getLocalPlayer()->getCAO();
	if (!camera)
		return 0;
	sanity_check(playercao);
	if (!lua_isnumber(L, 2))
		return 0;

	camera->setCameraMode((CameraMode)((int)lua_tonumber(L, 2)));
	// Make the player visible depending on camera mode.
	playercao->updateMeshCulling();
	playercao->setChildrenVisible(camera->getCameraMode() > CAMERA_MODE_FIRST);
	return 0;
}

// get_camera_mode(self)
int LuaCamera::l_get_camera_mode(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_pushinteger(L, (int)camera->getCameraMode());

	return 1;
}

// get_fov(self)
int LuaCamera::l_get_fov(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_newtable(L);
	lua_pushnumber(L, camera->getFovX() * core::RADTODEG);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, camera->getFovY() * core::RADTODEG);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, camera->getCameraNode()->getFOV() * core::RADTODEG);
	lua_setfield(L, -2, "actual");
	lua_pushnumber(L, camera->getFovMax() * core::RADTODEG);
	lua_setfield(L, -2, "max");
	return 1;
}

// get_pos(self)
int LuaCamera::l_get_pos(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	push_v3f(L, camera->getPosition() / BS);
	return 1;
}

// get_offset(self)
int LuaCamera::l_get_offset(lua_State *L)
{
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	sanity_check(player);

	push_v3f(L, player->getEyeOffset() / BS);
	return 1;
}

// set_offset(self, offset)
int
LuaCamera::l_set_offset (lua_State *L)
{
  LocalPlayer *player = getClient (L)->getEnv ().getLocalPlayer ();
  sanity_check (player);

  if (!lua_isnil (L, 2))
    {
      v3f offset = readParam<v3f> (L, 2, v3f (0)) * BS;
      player->overrideEyeOffset (&offset);
    }
  else
    player->overrideEyeOffset (nullptr);
  return 1;
}

// get_look_dir(self)
int LuaCamera::l_get_look_dir(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	push_v3f(L, camera->getDirection());
	return 1;
}

// get_look_horizontal(self)
// FIXME: wouldn't localplayer be a better place for this?
int LuaCamera::l_get_look_horizontal(lua_State *L)
{
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	sanity_check(player);

	lua_pushnumber(L, player->getYaw() * core::DEGTORAD);
	return 1;
}

// get_look_vertical(self)
// FIXME: wouldn't localplayer be a better place for this?
int LuaCamera::l_get_look_vertical(lua_State *L)
{
	LocalPlayer *player = getClient(L)->getEnv().getLocalPlayer();
	sanity_check(player);

	lua_pushnumber(L, -1.0f * player->getPitch() * core::DEGTORAD);
	return 1;
}

int
LuaCamera::l_set_look_horizontal (lua_State *L)
{
  LocalPlayer *player = getClient (L)->getEnv ().getLocalPlayer ();
  float yaw;

  sanity_check (player);
  yaw = (readParam<float> (L, 2)) * core::RADTODEG;
  player->setYaw (yaw);
  return 1;
}

int
LuaCamera::l_set_look_vertical (lua_State *L)
{
  LocalPlayer *player = getClient (L)->getEnv ().getLocalPlayer ();
  float pitch;

  sanity_check (player);
  pitch = readParam<float> (L, 2) * core::RADTODEG;
  player->setPitch (pitch);
  return 1;
}

// get_aspect_ratio(self)
int LuaCamera::l_get_aspect_ratio(lua_State *L)
{
	Camera *camera = getobject(L, 1);
	if (!camera)
		return 0;

	lua_pushnumber(L, camera->getCameraNode()->getAspectRatio());
	return 1;
}

// update_wield_item (player, transition)

int
LuaCamera::l_update_wield_item (lua_State *L)
{
  LocalPlayer *player = getClient (L)->getEnv ().getLocalPlayer ();
  Camera *camera = getobject (L, 1);

  sanity_check (player);
  if (!camera)
    return 0;

  {
    ItemStack selected_item, hand_item;
    ItemStack &tool_item
      = player->getWieldedItem (&selected_item, &hand_item);
    camera->wield (tool_item, lua_toboolean (L, 2));
  }
  return 0;
}

// override_wieldmesh (pos, rot, transition_time)

int
LuaCamera::l_override_wieldmesh (lua_State *L)
{
  Camera *camera = getobject (L, 1);
  v3f pos = readParam<v3f> (L, 2, v3f (0));
  v3f rot = readParam<v3f> (L, 3, v3f (0));
  f32 transition = readParam<float> (L, 4, 0.0f);

  sanity_check (camera);
  if (!camera)
    return 0;

  camera->overrideWieldmesh (&pos, &rot, std::max (0.0f, transition));
  return 0;
}

// reset_wieldmesh_override (transition)

int
LuaCamera::l_reset_wieldmesh_override (lua_State *L)
{
  Camera *camera = getobject (L, 1);
  f32 transition = readParam<float> (L, 2, 0.0f);
  camera->resetWieldmeshOverride (std::max (0.0f, transition));
  return 0;
}

// set_ambient_lighting (light_level, range_squeeze)

int
LuaCamera::l_set_ambient_lighting (lua_State *L)
{
  Client *client = getClient (L);
  ClientMap *map = &client->getEnv ().getClientMap ();
  int lighting = readParam<int> (L, 2, 0);
  int squeeze = readParam<int> (L, 3, 0);
  map->setAmbientLight (lighting, squeeze);
  return 0;
}

// set_sky (skybox)

int
LuaCamera::l_set_sky (lua_State *L)
{
  SkyboxParams sky_params
    = SkyboxDefaults::getSkyDefaults ();
  ClientEvent *event;

  if (!lua_isnoneornil (L, 2))
    {
      luaL_checktype (L, 2, LUA_TTABLE);
      lua_getfield (L, 2, "base_color");
      if (!lua_isnil (L, -1))
	read_color (L, -1, &sky_params.bgcolor);
      lua_pop (L, 1);

      lua_getfield (L, 2, "body_orbit_tilt");
      if (!lua_isnil (L, -1))
	sky_params.body_orbit_tilt
	  = rangelim (readParam <float> (L, -1), -60.0f, 60.0f);
      lua_pop (L, 1);

      lua_getfield (L, 2, "type");
      if (!lua_isnil (L, -1))
	sky_params.type = luaL_checkstring (L, -1);
      lua_pop (L, 1);

      lua_getfield (L, 2, "textures");
      sky_params.textures.clear ();
      if (lua_istable (L, -1) && sky_params.type == "skybox")
	{
	  lua_pushnil (L);
	  while (lua_next (L, -2) != 0)
	    {
	      // Key is at index -2 and value at index -1
	      sky_params.textures.emplace_back (readParam <std::string> (L, -1));
	      // Removes the value, but keeps the key for iteration
	      lua_pop (L, 1);
	    }
	}
      lua_pop (L, 1);

      // Validate that we either have six or zero textures
      if (sky_params.textures.size () != 6 && !sky_params.textures.empty ())
	throw LuaError ("Skybox expects 6 textures!");

      sky_params.clouds
	= getboolfield_default (L, 2, "clouds", sky_params.clouds);

      lua_getfield (L, 2, "sky_color");
      if (lua_istable (L, -1))
	{
	  lua_getfield (L, -1, "day_sky");
	  read_color (L, -1, &sky_params.sky_color.day_sky);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "day_horizon");
	  read_color (L, -1, &sky_params.sky_color.day_horizon);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "dawn_sky");
	  read_color (L, -1, &sky_params.sky_color.dawn_sky);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "dawn_horizon");
	  read_color (L, -1, &sky_params.sky_color.dawn_horizon);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "night_sky");
	  read_color (L, -1, &sky_params.sky_color.night_sky);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "night_horizon");
	  read_color (L, -1, &sky_params.sky_color.night_horizon);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "indoors");
	  read_color (L, -1, &sky_params.sky_color.indoors);
	  lua_pop (L, 1);

	  // Prevent flickering clouds at dawn/dusk:
	  sky_params.fog_sun_tint = video::SColor (255, 255, 255, 255);
	  lua_getfield (L, -1, "fog_sun_tint");
	  read_color (L, -1, &sky_params.fog_sun_tint);
	  lua_pop (L, 1);

	  sky_params.fog_moon_tint = video::SColor (255, 255, 255, 255);
	  lua_getfield (L, -1, "fog_moon_tint");
	  read_color (L, -1, &sky_params.fog_moon_tint);
	  lua_pop (L, 1);

	  lua_getfield (L, -1, "fog_tint_type");
	  if (!lua_isnil (L, -1))
	    sky_params.fog_tint_type = luaL_checkstring (L, -1);
	  lua_pop (L, 1);
	}
      lua_pop (L, 1);

      lua_getfield (L, 2, "fog");
      if (lua_istable (L, -1))
	{
	  sky_params.fog_distance = getintfield_default (L, -1,
							 "fog_distance",
							 sky_params.
							 fog_distance);
	  sky_params.fog_start
	    = getfloatfield_default (L, -1, "fog_start", sky_params.fog_start);

	  lua_getfield (L, -1, "fog_color");
	  read_color (L, -1, &sky_params.fog_color);
	  lua_pop (L, 1);
	}
      lua_pop (L, 1);
    }

  event = new ClientEvent ();
  event->type = CE_SET_SKY;
  event->set_sky = new SkyboxParams (sky_params);
  getClient (L)->pushToEventQueue (event);
  return 0;
}

// set_moon (moon_params)

int
LuaCamera::l_set_moon (lua_State *L)
{
  MoonParams moon_params = SkyboxDefaults::getMoonDefaults ();
  ClientEvent *event;

  if (!lua_isnoneornil (L, 2))
    {
      luaL_checktype (L, 2, LUA_TTABLE);
      moon_params.visible
	= getboolfield_default (L, 2, "visible", moon_params.visible);
      moon_params.texture
	= getstringfield_default (L, 2, "texture", moon_params.texture);
      moon_params.tonemap
	= getstringfield_default (L, 2, "tonemap", moon_params.tonemap);
      moon_params.scale
	= getfloatfield_default (L, 2, "scale", moon_params.scale);
    }

  event = new ClientEvent ();
  event->type = CE_SET_MOON;
  event->moon_params = new MoonParams (moon_params);
  getClient (L)->pushToEventQueue (event);
  return 0;
}

// set_sun (sun_params)

int
LuaCamera::l_set_sun (lua_State *L)
{
  SunParams sun_params = SkyboxDefaults::getSunDefaults ();
  ClientEvent *event;

  if (!lua_isnoneornil (L, 2))
    {
      luaL_checktype (L, 2, LUA_TTABLE);
      sun_params.visible
	= getboolfield_default (L, 2, "visible", sun_params.visible);
      sun_params.texture
	= getstringfield_default (L, 2, "texture", sun_params.texture);
      sun_params.tonemap
	= getstringfield_default (L, 2, "tonemap", sun_params.tonemap);
      sun_params.sunrise
	= getstringfield_default (L, 2, "sunrise", sun_params.sunrise);
      sun_params.sunrise_visible
	= getboolfield_default (L, 2, "sunrise_visible", sun_params.sunrise_visible);
      sun_params.scale
	= getfloatfield_default (L, 2,  "scale", sun_params.scale);
    }

  event = new ClientEvent ();
  event->type = CE_SET_SUN;
  event->sun_params = new SunParams (sun_params);
  getClient (L)->pushToEventQueue (event);
  return 0;
}

// set_stars (star_params)

int
LuaCamera::l_set_stars (lua_State *L)
{
  StarParams star_params = SkyboxDefaults::getStarDefaults ();
  ClientEvent *event;

  if (!lua_isnoneornil (L, 2))
    {
      luaL_checktype (L, 2, LUA_TTABLE);
      star_params.visible
	= getboolfield_default(L, 2, "visible", star_params.visible);
      star_params.count
	= getintfield_default(L, 2,  "count",   star_params.count);

      lua_getfield (L, 2, "star_color");
      if (!lua_isnil (L, -1))
	read_color (L, -1, &star_params.starcolor);
      lua_pop (L, 1);
      star_params.scale
	= getfloatfield_default (L, 2, "scale", star_params.scale);
      star_params.day_opacity
	= getfloatfield_default (L, 2, "day_opacity", star_params.day_opacity);
    }

  event = new ClientEvent ();
  event->type = CE_SET_STARS;
  event->star_params = new StarParams (star_params);
  getClient (L)->pushToEventQueue (event);
  return 0;
}


Camera *LuaCamera::getobject(LuaCamera *ref)
{
	return ref->m_camera;
}

Camera *LuaCamera::getobject(lua_State *L, int narg)
{
	LuaCamera *ref = checkObject<LuaCamera>(L, narg);
	assert(ref);
	return getobject(ref);
}

int LuaCamera::gc_object(lua_State *L)
{
	LuaCamera *o = *(LuaCamera **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

void LuaCamera::Register(lua_State *L)
{
	static const luaL_Reg metamethods[] = {
		{"__gc", gc_object},
		{0, 0}
	};
	registerClass<LuaCamera>(L, methods, metamethods);
}

const char LuaCamera::className[] = "Camera";
const luaL_Reg LuaCamera::methods[] = {
	luamethod(LuaCamera, set_camera_mode),
	luamethod(LuaCamera, get_camera_mode),
	luamethod(LuaCamera, get_fov),
	luamethod(LuaCamera, get_pos),
	luamethod(LuaCamera, get_offset),
	luamethod(LuaCamera, set_offset),
	luamethod(LuaCamera, get_look_dir),
	luamethod(LuaCamera, get_look_vertical),
	luamethod(LuaCamera, get_look_horizontal),
	luamethod(LuaCamera, get_aspect_ratio),
	luamethod(LuaCamera, set_look_vertical),
	luamethod(LuaCamera, set_look_horizontal),
	luamethod(LuaCamera, update_wield_item),
	luamethod (LuaCamera, override_wieldmesh),
	luamethod (LuaCamera, reset_wieldmesh_override),
	luamethod (LuaCamera, set_ambient_lighting),
	luamethod (LuaCamera, set_sky),
	luamethod (LuaCamera, set_moon),
	luamethod (LuaCamera, set_sun),
	luamethod (LuaCamera, set_stars),

	{0, 0}
};
