function core.setting_get_pos(name)
	local value = core.settings:get(name)
	if not value then
		return nil
	end
	return core.string_to_pos(value)
end


-- old non-method sound functions

function core.sound_stop(handle, ...)
	return handle:stop(...)
end

function core.sound_fade(handle, ...)
	return handle:fade(...)
end

-- Helper that pushes a collisionMoveResult structure
if core.set_push_moveresult1 then
	-- must match CollisionAxis in collision.h
	local AXES = {"x", "y", "z"}
	-- <=> script/common/c_content.cpp push_collision_move_result()
	core.set_push_moveresult1(function(b0, b1, b2, axis, npx, npy, npz, v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z)
		return {
			touching_ground = b0,
			collides = b1,
			standing_on_object = b2,
			collisions = {{
				type = "node",
				axis = AXES[axis + 1],
				node_pos = vector.new(npx, npy, npz),
				new_pos = vector.new(v0x, v0y, v0z),
				old_velocity = vector.new(v1x, v1y, v1z),
				new_velocity = vector.new(v2x, v2y, v2z),
			}},
		}
	end)
	core.set_push_moveresult1 = nil
end
