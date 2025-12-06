// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include "irrlichttypes_bloated.h"
#include "irr_ptr.h"
#include "ISceneNode.h"
#include "S3DVertex.h"
#include "CMeshBuffer.h"

#include <mutex>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../particles.h"
#include "util/numeric.h"

namespace video {
	class ITexture;
}

struct ClientEvent;
class ParticleManager;
class ClientEnvironment;
struct MapNode;
struct ContentFeatures;
class LocalPlayer;
class ITextureSource;
class IGameDef;
class Client;

struct ClientParticleTexture
{
	/* per-spawner structure used to store the ParticleTexture structs
	 * that spawned particles will refer to through ClientParticleTexRef */
	ParticleTexture tex;
	video::ITexture *ref = nullptr;

	ClientParticleTexture() = default;
	ClientParticleTexture(const ServerParticleTexture& p, ITextureSource *tsrc);
};

struct ClientParticleTexRef
{
	/* per-particle structure used to avoid massively duplicating the
	 * fairly large ParticleTexture struct */
	ParticleTexture *tex = nullptr;
	video::ITexture *ref = nullptr;

	ClientParticleTexRef() = default;

	/* constructor used by particles spawned from a spawner */
	explicit ClientParticleTexRef(ClientParticleTexture &t):
			tex(&t.tex), ref(t.ref) {};

	/* constructor used for node particles */
	explicit ClientParticleTexRef(video::ITexture *tp): ref(tp) {};
};

class ParticleSpawner;
class ParticleBuffer;

class Particle
{
public:
	Particle(
		const ParticleParameters &p,
		const ClientParticleTexRef &texture,
		v2f texpos,
		v2f texsize,
		video::SColor color,
		ParticleSpawner *parent = nullptr,
		std::unique_ptr<ClientParticleTexture> owned_texture = nullptr
	);

	~Particle();

	DISABLE_CLASS_COPY(Particle)

	void step(float dtime, ClientEnvironment *env);

	bool isExpired () const
	{ return m_expiration < m_time; }

	ParticleSpawner *getParent() const { return m_parent; }

	const ClientParticleTexRef &getTextureRef() const { return m_texture; }

	ParticleParamTypes::BlendMode getBlendMode() const
	{ return m_texture.tex ? m_texture.tex->blendmode : m_p.texture.blendmode; }

	ParticleBuffer *getBuffer() const { return m_buffer; }
	bool attachToBuffer(ParticleBuffer *buffer);

private:
	video::SColor updateLight(ClientEnvironment *env);
	void updateVertices(ClientEnvironment *env, video::SColor color);

	ParticleBuffer *m_buffer = nullptr;
	u16 m_index; // index in m_buffer

	float m_time = 0.0f;
	float m_expiration;

	// Color without lighting
	video::SColor m_base_color;

	ClientParticleTexRef m_texture;
	v2f m_texpos;
	v2f m_texsize;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;

	const ParticleParameters m_p;

	float m_animation_time = 0.0f;
	int m_animation_frame = 0;

	ParticleSpawner *m_parent = nullptr;
	// Used if not spawned from a particlespawner
	std::unique_ptr<ClientParticleTexture> m_owned_texture;
};

class ParticleSpawner
{
public:
	ParticleSpawner(LocalPlayer *player,
		const ParticleSpawnerParameters &params,
		u16 attached_id,
		std::vector<ClientParticleTexture> &&texpool,
		ParticleManager *p_manager);

	void step(float dtime, ClientEnvironment *env);

	bool getExpired() const
	{ return p.amount <= 0 && p.time != 0; }

	bool hasActive() const { return m_active != 0; }
	void decrActive() { m_active -= 1; }

private:
	void spawnParticle(ClientEnvironment *env, float radius,
		const core::matrix4 *attached_absolute_pos_rot_matrix);

	size_t m_active;
	ParticleManager *m_particlemanager;
	float m_time;
	LocalPlayer *m_player;
	ParticleSpawnerParameters p;
	std::vector<ClientParticleTexture> m_texpool;
	std::vector<float> m_spawntimes;
	u16 m_attached_id;
};

class ParticleBuffer : public scene::ISceneNode
{
	friend class ParticleManager;
public:
	ParticleBuffer(ClientEnvironment *env, const video::SMaterial &material);

	// for pointer stability
	DISABLE_CLASS_COPY(ParticleBuffer)

	/// Reserves one more slot for a particle (4 vertices, 6 indices)
	/// @return particle index within buffer
	std::optional<u16> allocate();
	/// Frees the particle at `index`
	void release(u16 index);
	typedef std::vector<u16>::const_iterator u16_const_iterator;
	void release_bulk (u16_const_iterator, u16_const_iterator);
	void clear_slots (u16 *, ptrdiff_t, ptrdiff_t);
	void enable_slots (u16 *, u16, u16);

	/// @return video::S3DVertex[4]
	video::S3DVertex *getVertices(u16 index);

	inline bool isEmpty() const {
		return m_free_list.size() == m_count;
	}

	virtual video::SMaterial &getMaterial(u32 num) override {
		return m_mesh_buffer->getMaterial();
	}
	virtual u32 getMaterialCount() const override {
		return 1;
	}

	inline void use (void) { m_usage_timer = 0.0f; };

	virtual const core::aabbox3df &getBoundingBox() const override;

	virtual void render() override;

	virtual void OnRegisterSceneNode() override;

	// we have 16-bit indices
	static constexpr u16 MAX_PARTICLES_PER_BUFFER = 16000;

private:
	irr_ptr<scene::SMeshBuffer> m_mesh_buffer;
	// unused (e.g. expired) particle indices for re-use
	std::vector<u16> m_free_list;
	// for automatic deletion when unused for a while. is reset on allocate().
	float m_usage_timer = 0;
	// total count of contained particles
	u16 m_count = 0;
	mutable bool m_bounding_box_dirty = true;
};

typedef short heightmap_block[MAP_BLOCKSIZE * MAP_BLOCKSIZE];
class VolumeParticleSpawner;

extern "C"
{
  struct ColumnVisibilityMap
  {
    unsigned int *flags;
    int range;
    s16 cx, cz;
  };
}

struct buffer_slot_cache
{
  /* If this texture slot's material is shared with another, pointer
     to that material's buffer slot cache, or NULL otherwise.  The
     subsequent fields are invalid if this is set.  */
  struct buffer_slot_cache *indirect;

  /* The particle buffer to which this cache refers.  */
  ParticleBuffer *buffer;

  /* Size of this cache's data, in elements.  */
  size_t size;

  /* Indices of the first available slot in this cache and the slot
     after the last slot defined, respectively.  */
  ptrdiff_t i, head;

  /* Pointer to SIZE elements holding HEAD offsets into the vertex
     buffer.  */
  u16 *data;
};

typedef struct buffer_slot_cache *buffer_slot_list;

class VolumeParticleSpawner
{
public:
  VolumeParticleSpawner () = default;
  VolumeParticleSpawner (VolumeParticleSpawner &&) = default;
  VolumeParticleSpawner &operator= (VolumeParticleSpawner &&) = default;

  std::vector<ClientParticleTexture> m_texpool;
  std::vector<video::SMaterial> m_materials;
  buffer_slot_list m_slots = NULL;

  u64 id;

  struct ColumnVisibilityMap *visibility_map;
  unsigned int visibility_test;

  video::SColor color = video::SColor (255, 255, 255, 255);
  v2f texpos = v2f (0.0f, 0.0f);
  v2f texsize = v2f (1.0f, 1.0f);

  v3f velocity_min;
  v3f velocity_max;
  float period;
  float size;
  float sx, sy;

  s16 particles_per_column;
  s16 range_horizontal;
  s16 range_vertical;

  bool above_heightmap_p : 1; /* TODO */
};

class ClientMap;
struct VolumeParticleData;

/**
 * Class doing particle as well as their spawners handling
 */
class ParticleManager
{
	friend class ParticleSpawner;
public:
	ParticleManager(ClientEnvironment* env, Client *);
	DISABLE_CLASS_COPY(ParticleManager)
	~ParticleManager();

	void step (float dtime);

	void handleParticleEvent(ClientEvent *event, Client *client,
			LocalPlayer *player);

	void addDiggingParticles(LocalPlayer *player, v3s16 pos,
		const MapNode &n);

	void addNodeParticle(LocalPlayer *player, v3s16 pos,
		const MapNode &n);

	void reserveParticleSpace(size_t max_estimate);

	/**
	 * This function is only used by client particle spawners
	 *
	 * We don't need to check the particle spawner list because client ID will
	 * never overlap (u64)
	 * @return new id
	 */
	u64 generateSpawnerId()
	{
		return m_next_particle_spawner_id++;
	}

  u64 add_volume_particle_spawner (VolumeParticleSpawner *);
  bool delete_volume_particle_spawner (u64, bool);
  bool set_column_visibility_map (u64, struct ColumnVisibilityMap *, unsigned int);

protected:
	static bool getNodeParticleParams(Client *client, const MapNode &n,
		ParticleParameters &p, video::ITexture **texture, v2f &texpos,
		v2f &texsize, video::SColor *color, u8 tilenum = 0);

	static video::SMaterial getMaterialForParticle(const Particle *texture);

	bool addParticle(std::unique_ptr<Particle> toadd);

private:
	void addParticleSpawner(u64 id, std::unique_ptr<ParticleSpawner> toadd);
	void deleteParticleSpawner(u64 id);

	void stepParticles(float dtime);
	void stepSpawners(float dtime);
	void stepBuffers(float dtime);

  ParticleBuffer *find_particle_buffer (video::SMaterial &);

  void add_volume_particle (VolumeParticleSpawner *, ClientMap &,
			    struct VolumeParticleData *);
  void prepare_volume_spawner (VolumeParticleSpawner *);
  void initialize_volume_spawner (VolumeParticleSpawner *);
  void step_volume_spawners (float);
  void delete_particle_buffer (ParticleBuffer *);
  bool delete_volume_particle_spawner_1 (u64, bool);

	void clearAll();

	std::vector<std::unique_ptr<Particle>> m_particles;
	std::unordered_map<u64, std::unique_ptr<ParticleSpawner>> m_particle_spawners;
	std::vector<std::unique_ptr<ParticleSpawner>> m_dying_particle_spawners;
	std::vector<irr_ptr<ParticleBuffer>> m_particle_buffers;

	// Start the particle spawner ids generated from here after u32_max.
	// lower values are for server sent spawners.
	u64 m_next_particle_spawner_id = static_cast<u64>(U32_MAX) + 1;

	ClientEnvironment *m_env;

	IntervalLimiter m_buffer_gc;

	std::mutex m_particle_list_lock;
	std::mutex m_spawner_list_lock;

  /* Volume particle spawner data.  */
  double time_elapsed = 0.0;
  u64 last_volume_spawner_id = 0;
  std::unordered_map<u64, VolumeParticleSpawner *> volume_spawners;

  Client *client;
};
