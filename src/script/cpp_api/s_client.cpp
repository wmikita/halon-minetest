// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#include "s_client.h"
#include "s_internal.h"
#include "client/client.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "lua_api/l_item.h"
#include "itemdef.h"
#include "s_item.h"
#include "client/localplayer.h"
#include "lua_api/l_client_object.h"
#include "profiler.h"

ScriptApiClient::~ScriptApiClient ()
{
  if (getEnv ())
    {
      ClientEnvironment *env = static_cast<ClientEnvironment *> (getEnv ());
      env->setScript (nullptr);
    }
}

void ScriptApiClient::on_mods_loaded()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_mods_loaded");
	// Call callbacks
	try {
		runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
	}
}

void ScriptApiClient::on_shutdown()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	try {
		runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
	}
}

bool ScriptApiClient::on_sending_message(const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_sending_chat_message");
	// Call callbacks
	lua_pushstring(L, message.c_str());
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_receiving_message(const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_receiving_chat_message");
	// Call callbacks
	lua_pushstring(L, message.c_str());
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return readParam<bool>(L, -1);
}

void ScriptApiClient::on_damage_taken(int32_t damage_amount)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_damage_taken");
	// Call callbacks
	lua_pushinteger(L, damage_amount);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
	}
}

void ScriptApiClient::on_hp_modification(int32_t newhp)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_hp_modification");
	// Call callbacks
	lua_pushinteger(L, newhp);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_OR_SC);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
	}
}

void ScriptApiClient::environment_step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
	}
}

bool ScriptApiClient::on_dignode(v3s16 p, MapNode node)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_dignode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_dignode");

	// Push data
	push_v3s16(L, p);
	pushnode(L, node);

	// Call functions
	try {
		runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return lua_toboolean(L, -1);
}

bool ScriptApiClient::on_punchnode(v3s16 p, MapNode node)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_punchgnode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_punchnode");

	// Push data
	push_v3s16(L, p);
	pushnode(L, node);

	// Call functions
	try {
		runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_placenode(const PointedThing &pointed, const ItemDefinition &item)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_placenode
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_placenode");

	// Push data
	push_pointed_thing(L, pointed, true);
	push_item_definition(L, item);

	// Call functions
	try {
		runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_item_use(const ItemStack &item, const PointedThing &pointed)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_item_use
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_item_use");

	// Push data
	LuaItemStack::create(L, item);
	push_pointed_thing(L, pointed, true);

	// Call functions
	try {
		runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return readParam<bool>(L, -1);
}

bool
ScriptApiClient::on_item_place (const ItemStack &item, const PointedThing &pointed)
{
  SCRIPTAPI_PRECHECKHEADER

  // Get core.registered_on_item_use
  lua_getglobal (L, "core");
  lua_getfield (L, -1, "registered_on_item_place");

  // Push data
  LuaItemStack::create (L, item);
  push_pointed_thing (L, pointed, true);

  // Call functions
  try {
    runCallbacks(2, RUN_CALLBACKS_MODE_OR);
  } catch (LuaError &e) {
    getClient()->setFatalError(e);
    return true;
  }
  return readParam<bool>(L, -1);
}

bool ScriptApiClient::on_inventory_open(Inventory *inventory)
{
	SCRIPTAPI_PRECHECKHEADER

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_inventory_open");

	push_inventory_lists(L, *inventory);

	try {
		runCallbacks(1, RUN_CALLBACKS_MODE_OR);
	} catch (LuaError &e) {
		getClient()->setFatalError(e);
		return true;
	}
	return readParam<bool>(L, -1);
}

void
ScriptApiClient::on_teleport_localplayer (const v3f &new_pos)
{
  SCRIPTAPI_PRECHECKHEADER

  lua_getglobal (L, "core");
  lua_getfield (L, -1, "registered_on_teleport_localplayer");
  push_v3f (L, new_pos);
  try
    {
      runCallbacks (1, RUN_CALLBACKS_MODE_OR);
    }
  catch (LuaError &e)
    {
      getClient()->setFatalError (e);
    }
}

void
ScriptApiClient::on_localplayer_object_available (void)
{
  SCRIPTAPI_PRECHECKHEADER
  lua_getglobal (L, "core");
  lua_getfield (L, -1, "registered_on_localplayer_object_available");
  try
    {
      runCallbacks (0, RUN_CALLBACKS_MODE_OR);
    }
  catch (LuaError &e)
    {
      getClient ()->setFatalError (e);
      return;
    }
}

void ScriptApiClient::setEnv(ClientEnvironment *env)
{
	ScriptApiBase::setEnv(env);
}

bool
ScriptApiClient::callOnMove (LocalPlayer *player, f32 dtime, Environment *env,
			     std::vector<CollisionInfo> *collision_info)
{
  SCRIPTAPI_PRECHECKHEADER
  int error_handler;

  error_handler = PUSH_ERROR_HANDLER (L);
  lua_getglobal (L, "core");
  luaL_checktype (L, -1, LUA_TTABLE);
  lua_getfield (L, -1, "player_callbacks");

  if (!lua_istable (L, -1))
    return false;

  lua_getfield (L, -1, "on_step");
  if (lua_isnil (L, -1))
    return false;
  luaL_checktype (L, -1, LUA_TFUNCTION);

  /* Begin slimmed down `LocalPlayer->move'.  This is expected to push
     dtime, moveresult, and flags.  */
  if (!player->luaMove (L, dtime, env, collision_info))
    return true;

  {
    ScopeProfiler sp (g_profiler, "Client: LocalPlayer on_step handler",
		      SPT_AVG, PRECISION_MICRO);
    PCALL_RES (lua_pcall (L, 3, 0, error_handler));
  }
  return true;
}

void
ScriptApiClient::removeClientObjectReference (ClientActiveObject *cao)
{
  SCRIPTAPI_PRECHECKHEADER
  int top;

  assert (getType () == ScriptingType::Client);
  top = lua_gettop (L);
  lua_getglobal (L, "core");
  lua_getfield (L, -1, "client_object_refs");
  luaL_checktype (L, -1, LUA_TTABLE);
  lua_pushinteger (L, cao->getId ());
  lua_gettable (L, -2);

  if (!lua_isnil (L, -1))
    {
      ClientObjectRef::set_null (L, (void *) cao);
      lua_pushinteger (L, cao->getId ());
      lua_pushnil (L);
      lua_settable (L, -4);
    }

  lua_settop (L, top);
}

// Local Variables:
// c-noise-macro-names: ("SCRIPTAPI_PRECHECKHEADER")
// End:
