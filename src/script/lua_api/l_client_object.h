// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2024 Luanti developers

#ifndef L_CLIENT_OBJECT_H
#define L_CLIENT_OBJECT_H

#include "lua_api/l_base.h"
#include "irrlichttypes.h"

class ClientActiveObject;
class GenericCAO;

class ClientObjectRef : public ModApiBase
{
public:
  ClientObjectRef (ClientActiveObject *);
  ~ClientObjectRef () = default;

  ClientActiveObject *get_object (void) { return m_object; };

  static void create (lua_State *, ClientActiveObject *);
  static void set_null (lua_State *, void *);
  static void clientObjectRefGetOrCreate (lua_State *, ClientActiveObject *);

  static void Register (lua_State *);
  static const char className[];

private:
  ClientActiveObject *m_object = nullptr;
  static luaL_Reg methods[];

  static GenericCAO *getgenericcao (ClientObjectRef *);

  // Exported functions.

  // garbage collector
  static int gc_object (lua_State *);

  // is_valid (self)
  static int l_is_valid (lua_State *);

  // set_property_overrides (table)
  static int l_set_property_overrides (lua_State *);

  // clear_property_overrides (list)
  static int l_clear_property_overrides (lua_State *);

  // get_properties ()
  static int l_get_properties (lua_State *);

  // set_animation (frame_range, frame_blend, frame_loop)
  static int l_set_animation (lua_State *);

  // set_animation_frame_speed (speed)
  static int l_set_animation_frame_speed (lua_State *);

  // set_bone_override (bone, value)
  static int l_set_bone_override (lua_State *);
};

#endif /* !L_CLIENT_OBJECT_H */
