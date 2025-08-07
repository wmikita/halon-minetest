// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Luanti project developers

#include "debug.h"
#include "lua_api/l_client_object.h"
#include "lua_api/l_internal.h"
#include "client/content_cao.h"
#include "common/c_content.h"
#include "object_properties.h"

/* ClientObjectRef.  */

ClientObjectRef::ClientObjectRef (ClientActiveObject *cao)
  : m_object (cao)
{
}

GenericCAO *
ClientObjectRef::getgenericcao (ClientObjectRef *ref)
{
  ClientActiveObject *cao = ref->m_object;

  if (!cao)
    return nullptr;

  return (cao->getType () == ACTIVEOBJECT_TYPE_GENERIC
	  ? (GenericCAO *) cao : nullptr);
}



/* Exported functions.  */

int
ClientObjectRef::gc_object (lua_State *L)
{
  ClientObjectRef *obj = *(ClientObjectRef **) (lua_touserdata (L, 1));
  delete obj;
  return 0;
}

int
ClientObjectRef::l_is_valid (lua_State *L)
{
  ClientObjectRef *obj = checkObject<ClientObjectRef> (L, 1);
  lua_pushboolean (L, obj->m_object != nullptr);
  return 1;
}

static int
parse_property_override_table (lua_State *L, ObjectProperties *props)
{
  return read_object_properties (L, 1.0, nullptr, props, nullptr);
}

int
ClientObjectRef::l_set_property_overrides (lua_State *L)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef> (L, 1);
  GenericCAO *cao = getgenericcao (ref);
  int flags;
  ObjectProperties props;

  if (!cao)
    return 0;

  flags = parse_property_override_table (L, &props);
  cao->overrideObjectProperties (props, flags);
  return 1;
}

int
ClientObjectRef::l_clear_property_overrides (lua_State *L)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef> (L, 1);
  GenericCAO *cao = getgenericcao (ref);
  int flags;

  if (!cao)
    return 0;

  luaL_checktype (L, -1, LUA_TTABLE);
  flags = flags_from_object_prop_list (L);
  cao->clearPropertyOverrides (flags);
  return 0;
}

int
ClientObjectRef::l_get_properties (lua_State *L)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef> (L, 1);
  GenericCAO *cao = getgenericcao (ref);

  if (!cao)
    return 0;

  push_object_properties (L, &cao->getProperties ());
  return 1;
}

int
ClientObjectRef::l_set_animation (lua_State *L)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef> (L, 1);
  GenericCAO *cao = getgenericcao (ref);

  if (!cao)
    return 0;

  if (lua_isnil (L, 2))
    cao->resetAnimationParams ();
  else
    {
      v2f frame_range   = readParam<v2f> (L, 2, v2f (1, 1));
      float frame_blend = readParam<float> (L, 3, 0.0f);
      bool frame_loop   = readParam<bool> (L, 4, true);
      cao->overrideAnimationParams (frame_range, frame_blend,
				    frame_loop);
    }
  return 1;
}

int
ClientObjectRef::l_set_animation_frame_speed (lua_State *L)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef> (L, 1);
  GenericCAO *cao = getgenericcao (ref);

  if (!cao)
    return 0;

  if (lua_isnil (L, 2))
    cao->resetAnimationSpeed ();
  else
    {
      float frame_speed = readParam<float> (L, 2, 0.0f);
      cao->overrideAnimationSpeed (frame_speed);
    }
  return 1;
}

// set_bone_override(self, bone, override)
int
ClientObjectRef::l_set_bone_override(lua_State *L)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef>(L, 1);
  GenericCAO *cao = getgenericcao (ref);
  if (cao == NULL)
    return 0;

  std::string bone = readParam<std::string>(L, 2);
  BoneOverride props;

  if (lua_isnoneornil (L, 3))
    {
      cao->setBoneOverride (bone, NULL);
      return 0;
    }

  read_bone_override (L, 3, props);
  cao->setBoneOverride (bone, &props);
  return 0;
}

void
ClientObjectRef::create (lua_State *L, ClientActiveObject *object)
{
  ClientObjectRef *ref = new ClientObjectRef (object);
  *(void **) (lua_newuserdata (L, sizeof (void *))) = ref;
  luaL_getmetatable (L, className);
  lua_setmetatable (L, -2);
}

void
ClientObjectRef::set_null (lua_State *L, void *expect)
{
  ClientObjectRef *ref = checkObject<ClientObjectRef> (L, -1);
  assert (ref);
  FATAL_ERROR_IF (ref->m_object != expect, "ObjectRef table was tampered with");
  ref->m_object = nullptr;
}

void
ClientObjectRef::Register(lua_State *L)
{
  static const luaL_Reg metamethods[] = {
    {"__gc", gc_object,},
    {0, 0,},
  };
  registerClass (L, className, methods, metamethods);
}

const char ClientObjectRef::className[] = "ClientObjectRef";
luaL_Reg ClientObjectRef::methods[] = {
  luamethod (ClientObjectRef, set_property_overrides),
  luamethod (ClientObjectRef, clear_property_overrides),
  luamethod (ClientObjectRef, get_properties),
  luamethod (ClientObjectRef, set_animation),
  luamethod (ClientObjectRef, set_animation_frame_speed),
  luamethod (ClientObjectRef, set_bone_override),
  {0, 0},
};



void
ClientObjectRef::clientObjectRefGetOrCreate (lua_State *L, ClientActiveObject *cao)
{
  lua_getglobal (L, "core");
  lua_getfield (L, -1, "client_object_refs");
  luaL_checktype (L, -1, LUA_TTABLE);
  lua_pushinteger (L, cao->getId ());
  lua_gettable (L, -2);

  if (lua_isnil (L, -1))
    {
      lua_pop (L, 3); /* core and client_object_refs and value.  */
      ClientObjectRef::create (L, cao);
      return;
    }

  lua_insert (L, -3);
  lua_pop (L, 2);
}
