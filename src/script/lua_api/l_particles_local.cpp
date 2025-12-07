// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 red-001 <red-001@outlook.ie>

#include "lua_api/l_particles_local.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_particleparams.h"
#include "client/particles.h"
#include "client/client.h"
#include "client/clientevent.h"

int ModApiParticlesLocal::l_add_particle(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	// Get parameters
	ParticleParameters p;

	lua_getfield(L, 1, "pos");
	if (lua_istable(L, -1))
		p.pos = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "velocity");
	if (lua_istable(L, -1))
		p.vel = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "acceleration");
	if (lua_istable(L, -1))
		p.acc = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "drag");
	if (lua_istable(L, -1))
		p.drag = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "jitter");
	LuaParticleParams::readLuaValue(L, p.jitter);
	lua_pop(L, 1);

	lua_getfield(L, 1, "bounce");
	LuaParticleParams::readLuaValue(L, p.bounce);
	lua_pop(L, 1);

	p.expirationtime = getfloatfield_default(L, 1, "expirationtime",
		p.expirationtime);
	p.size = getfloatfield_default(L, 1, "size", p.size);
	p.collisiondetection = getboolfield_default(L, 1,
		"collisiondetection", p.collisiondetection);
	p.collision_removal = getboolfield_default(L, 1,
		"collision_removal", p.collision_removal);
	p.object_collision = getboolfield_default(L, 1,
		"object_collision", p.object_collision);
	p.vertical = getboolfield_default(L, 1, "vertical", p.vertical);

	lua_getfield(L, 1, "animation");
	p.animation = read_animation_definition(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "texture");
	if (!lua_isnil(L, -1)) {
		LuaParticleParams::readTexValue(L,p.texture);
	}
	lua_pop(L, 1);
	p.glow = getintfield_default(L, 1, "glow", p.glow);

	lua_getfield(L, 1, "node");
	if (lua_istable(L, -1))
		p.node = readnode(L, -1);
	lua_pop(L, 1);

	p.node_tile = getintfield_default(L, 1, "node_tile", p.node_tile);

	ClientEvent *event = new ClientEvent();
	event->type           = CE_SPAWN_PARTICLE;
	event->spawn_particle = new ParticleParameters(p);
	getClient(L)->pushToEventQueue(event);

	return 0;
}

int ModApiParticlesLocal::l_add_particlespawner(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	// Get parameters
	ParticleSpawnerParameters p;
	p.amount = getintfield_default(L, 1, "amount", p.amount);
	p.time = getfloatfield_default(L, 1, "time", p.time);

	// set default values
	p.exptime = 1;
	p.size = 1;

	// read spawner parameters from the table
	using namespace ParticleParamTypes;
	LuaParticleParams::readTweenTable(L, "pos", p.pos);
	LuaParticleParams::readTweenTable(L, "vel", p.vel);
	LuaParticleParams::readTweenTable(L, "acc", p.acc);
	LuaParticleParams::readTweenTable(L, "size", p.size);
	LuaParticleParams::readTweenTable(L, "exptime", p.exptime);
	LuaParticleParams::readTweenTable(L, "drag", p.drag);
	LuaParticleParams::readTweenTable(L, "jitter", p.jitter);
	LuaParticleParams::readTweenTable(L, "bounce", p.bounce);
	lua_getfield(L, 1, "attract");
	if (!lua_isnil(L, -1)) {
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "kind");
		LuaParticleParams::readLuaValue(L, p.attractor_kind);
		lua_pop(L,1);

		lua_getfield(L, -1, "die_on_contact");
		if (!lua_isnil(L, -1))
			p.attractor_kill = readParam<bool>(L, -1);
		lua_pop(L,1);

		if (p.attractor_kind != AttractorKind::none) {
			LuaParticleParams::readTweenTable(L, "strength", p.attract);
			LuaParticleParams::readTweenTable(L, "origin", p.attractor_origin);
			p.attractor_attachment = LuaParticleParams::readAttachmentID(L, "origin_attached");
			if (p.attractor_kind != AttractorKind::point) {
				LuaParticleParams::readTweenTable(L, "direction", p.attractor_direction);
				p.attractor_direction_attachment = LuaParticleParams::readAttachmentID(L, "direction_attached");
			}
		}
	} else {
		p.attractor_kind = AttractorKind::none;
	}
	lua_pop(L,1);
	LuaParticleParams::readTweenTable(L, "radius", p.radius);

	p.collisiondetection = getboolfield_default(L, 1,
		"collisiondetection", p.collisiondetection);
	p.collision_removal = getboolfield_default(L, 1,
		"collision_removal", p.collision_removal);
	p.object_collision = getboolfield_default(L, 1,
		"object_collision", p.object_collision);

	lua_getfield(L, 1, "animation");
	p.animation = read_animation_definition(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "texture");
	if (!lua_isnil(L, -1)) {
		LuaParticleParams::readTexValue(L, p.texture);
	}
	lua_pop(L, 1);

	p.vertical = getboolfield_default(L, 1, "vertical", p.vertical);
	p.glow = getintfield_default(L, 1, "glow", p.glow);

	lua_getfield(L, 1, "texpool");
	if (lua_istable(L, -1)) {
		size_t tl = lua_objlen(L, -1);
		p.texpool.reserve(tl);
		for (size_t i = 0; i < tl; ++i) {
			lua_pushinteger(L, i+1), lua_gettable(L, -2);
			p.texpool.emplace_back();
			LuaParticleParams::readTexValue(L, p.texpool.back());
			lua_pop(L,1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, 1, "node");
	if (lua_istable(L, -1))
		p.node = readnode(L, -1);
	lua_pop(L, 1);

	p.node_tile = getintfield_default(L, 1, "node_tile", p.node_tile);

	u64 id = getClient(L)->getParticleManager()->generateSpawnerId();

	auto event = new ClientEvent();
	event->type                            = CE_ADD_PARTICLESPAWNER;
	event->add_particlespawner.p           = new ParticleSpawnerParameters(p);
	event->add_particlespawner.attached_id = 0;
	event->add_particlespawner.id          = id;

	getClient(L)->pushToEventQueue(event);
	lua_pushnumber(L, id);

	return 1;
}

int ModApiParticlesLocal::l_delete_particlespawner(lua_State *L)
{
	// Get parameters
	u32 id = luaL_checknumber(L, 1);

	ClientEvent *event = new ClientEvent();
	event->type                      = CE_DELETE_PARTICLESPAWNER;
	event->delete_particlespawner.id = id;

	getClient(L)->pushToEventQueue(event);
	return 0;
}

/* core.add_volume_particle_spawner (TABLE)

   Create a volume particle spawner and return its identifier.  A
   volume particle spawner is a system that arranges for a constant
   number of particles to fill each column in a rectangular volume
   around the camera, with apparently continuous motion achieved by
   deriving particle trajectories in a deterministic manner from game
   time and node position.

   TABLE describes the parameters of the volume particle spawner, and
   must be of the form:

   {
        -- List of textures from which particles will be selected.
        textures = { "foo.png", ... },

	-- Tint applied to each pixel in selected textures.
	color = "#ffffff",

	-- Velocity in nodes per second by which each particle
	-- will appear to move.
	velocity_min = vector.new (0.0, 0.0, 0.0),
	velocity_max = vector.new (0.0, 0.0, 0.0),

	-- The duration of a single particle's animation cycle, in
	-- seconds.  Each particle generated receives its position by
	-- taking the remainder of a division of the sum of the age of
	-- the game session with a random per-particle offset by this
	-- period quantity, and applying the velocity multiplied by
	-- the same to a random position within the base of the
	-- volume, wrapping the result's components around the
	-- boundaries of the column or volume as appropriate.  As
	-- such, the duration should be defined to a multiple of a
	-- value whose product with velocity yields a multiple of
	-- range_period, so as to prevent particles from appearing
	-- abruptly to teleport while in motion.
	period = 10.0,

	-- The size of each particle's billboard on screen.
	size = 1.0,

	-- Scaling factors applied to each particle.
	scale = { x = 1.0, y = 1.0, },

	-- The number of particles to generate in each column of the
	-- volume.
        particles_per_column = 24,

	-- The width and length in nodes of the area around the camera
        -- where columns of particles will be generated.
	range_horizontal = 7,

	-- The height of each column around the camera where columns
        -- of particles will generate.
	range_vertical = 7,

	-- Whether particles should only generate above the ground
        -- level as known to the client.
	above_heightmap = false,
   }  */

int
ModApiParticlesLocal::l_add_volume_particle_spawner (lua_State *L)
{
  Client *client = getClient (L);
  ParticleManager *manager = client->getParticleManager ();
  VolumeParticleSpawner spawner, *tem;

  luaL_checktype (L, 1, LUA_TTABLE);

  lua_getfield (L, 1, "textures");
  luaL_checktype (L, -1, LUA_TTABLE);
  lua_pushnil (L);
  while (lua_next (L, -2))
    {
      ServerParticleTexture texture;
      texture.string = luaL_checkstring (L, -1);
      spawner.m_texpool.emplace_back (texture, client->tsrc ());
      lua_pop (L, 1);
    }
  lua_pop (L, 1);

  if (spawner.m_texpool.empty ())
    {
      lua_pushstring (L, "texture pool list is empty");
      lua_error (L);
    }

  lua_getfield (L, 1, "color");
  if (!lua_isnil (L, -1))
    read_color (L, -1, &spawner.color);
  lua_pop (L, 1);

  lua_getfield (L, 1, "velocity_min");
  if (!lua_isnil (L, -1))
    spawner.velocity_min = check_v3f (L, -1);
  else
    spawner.velocity_min = v3f (0.0f, 0.0f, 0.0f);
  lua_pop (L, 1);

  lua_getfield (L, 1, "velocity_max");
  if (!lua_isnil (L, -1))
    spawner.velocity_max = check_v3f (L, -1);
  else
    spawner.velocity_max = v3f (0.0f, 0.0f, 0.0f);
  lua_pop (L, 1);

  lua_getfield (L, 1, "period");
  if (!lua_isnil (L, -1))
    spawner.period = luaL_checknumber (L, -1);
  else
    spawner.period = 10.0;
  lua_pop (L, 1);

  lua_getfield (L, 1, "size");
  if (!lua_isnil (L, -1))
    spawner.size = luaL_checknumber (L, -1);
  else
    spawner.size = 1.0;
  lua_pop (L, 1);

  lua_getfield (L, 1, "scale");
  if (!lua_isnil (L, -1))
    {
      v2f scale = check_v2f (L, -1);
      spawner.sx = scale.X;
      spawner.sy = scale.Y;
    }
  else
    spawner.sx = spawner.sy = 1.0;
  lua_pop (L, 1);

#define CHECK_INT_FIELD(name, def)		\
  lua_getfield (L, 1, #name);			\
  if (!lua_isnil (L, -1))			\
    spawner.name = luaL_checknumber (L, -1);	\
  else						\
    spawner.name = def;				\
  lua_pop (L, 1);

  CHECK_INT_FIELD (particles_per_column, 24);
  CHECK_INT_FIELD (range_horizontal, 7);
  CHECK_INT_FIELD (range_vertical, 7);
#undef CHECK_INT_FIELD

  spawner.range_horizontal = std::min (spawner.range_horizontal,
				       (s16) VOLUME_SPAWNER_RANGE_MAX);

  lua_getfield (L, 1, "above_heightmap");
  spawner.above_heightmap_p = lua_toboolean (L, -1);
  lua_pop (L, 2);

  /* Add the spawner.  */
  spawner.visibility_map = NULL;
  spawner.visibility_test = 0;
  tem = new VolumeParticleSpawner (std::move (spawner));
  lua_pushnumber (L, manager->add_volume_particle_spawner (tem));
  return 1;
}

int
ModApiParticlesLocal::l_delete_volume_particle_spawner (lua_State *L)
{
  u64 i = luaL_checknumber (L, -1);
  ParticleManager *mgr = getClient (L)->getParticleManager ();
  lua_pushboolean (L, mgr->delete_volume_particle_spawner (i, true));
  return 1;
}

int
ModApiParticlesLocal::l_set_volume_particle_spawner_visibility_map (lua_State *L)
{
  u64 id = luaL_checknumber (L, 1);
  s16 range = readParam<s16> (L, 2);
  if (range > 64)
    luaL_error (L, "Excessive range specified for particle spawner visibility map");
  else
    {
      ParticleManager *mgr = getClient (L)->getParticleManager ();
      s16 cx = readParam<s16> (L, 3);
      s16 cz = readParam<s16> (L, 4);
      unsigned int test = (int) luaL_checknumber (L, 6);
      size_t stride = range * 2 + 1, len = stride * stride;
      struct ColumnVisibilityMap *map;
      size_t size = sizeof *map + sizeof (int) * len;
      ptrdiff_t i;

      luaL_checktype (L, 5, LUA_TTABLE);
      if (lua_objlen (L, 5) != len)
	luaL_error (L, "Dimensions of provided visibility map data are incorrect");

      map = (struct ColumnVisibilityMap *) malloc (size);
      map->flags = (unsigned int *) (map + 1);
      lua_pushnil (L);
      for (i = 0; i < (ptrdiff_t) len; ++i)
	{
	  lua_next (L, 5);
	  map->flags[i] = lua_tonumber (L, -1);
	  lua_pop (L, 1);
	}
      lua_pop (L, 1);
      map->cx = cx;
      map->cz = cz;
      map->range = range;
      lua_pushboolean (L, mgr->set_column_visibility_map (id, map, test));
    }
  return 1;
}

void ModApiParticlesLocal::Initialize(lua_State *L, int top)
{
	API_FCT(add_particle);
	API_FCT(add_particlespawner);
	API_FCT(delete_particlespawner);
	API_FCT (add_volume_particle_spawner);
	API_FCT (delete_volume_particle_spawner);
	API_FCT (set_volume_particle_spawner_visibility_map);
}
