// -----------------------------------------------------------------------
// Types (typedef enum)
// -----------------------------------------------------------------------

typedef enum Layers {
	layer_background = 0,
	layer_world = 10,
	layer_ui = 20,
} Layers;

typedef enum TimedEventType {
	TIMED_EVENT_NIL = 0,

	TIMED_EVENT_COLOR_SWITCH = 1,
	TIMED_EVENT_BEAM = 2,
	TIMED_EVENT_BOSS_MOVEMENT = 3,
	TIMED_EVENT_BOSS_ATTACK = 4,
	TIMED_EVENT_BOSS_NEXT_STAGE = 5,
	TIMED_EVENT_EFFECT = 6,
	TIMED_EVENT_DROP = 7,
	TIMED_EVENT_PROJECTILE_LIFETIME = 8,

	TIMED_EVENT_MAX,
} TimedEventType;

typedef enum TimedEventWorldType {
	TIMED_EVENT_TYPE_NIL = 0,

	TIMED_EVENT_TYPE_WORLD = 1,
	TIMED_EVENT_TYPE_ENTITY = 2,

	TIMED_EVENT_TYPE_MAX,
} TimedEventWorldType;

typedef enum ParticleFlags {
    PARTICLE_FLAGS_valid = (1<<0),
	PARTICLE_FLAGS_physics = (1<<1),
	PARTICLE_FLAGS_friction = (1<<2),
	PARTICLE_FLAGS_fade_out_with_velocity = (1<<3),
	PARTICLE_FLAGS_gravity = (1<<4),
	PARTICLE_FLAGS_light = (1<<5),
	// PARTICLE_FLAGS_bounce = (1<<4),
} ParticleFlags;

typedef enum ParticleKind {
	PFX_NIL = 0,

	PFX_EFFECT = 1,
	PFX_BOUNCE = 2,
	PFX_HARD_OBSTACLE = 3,
	PFX_SNOW = 4,
	PFX_ASH = 5,
	PFX_LEAF = 6,
	PFX_RAIN = 7,
	PFX_WIND = 8,

	PFX_MAX,
} ParticleKind;

typedef enum ObstacleType {
	OBSTACLE_NIL = 0,

	OBSTACLE_BASE = 1,
	OBSTACLE_HARD = 2,
	OBSTACLE_BLOCK = 3,
	OBSTACLE_DROP = 4,
	OBSTACLE_BEAM = 6,

	OBSTACLE_MAX,
} ObstacleType;
	
typedef enum EffectSpawn {
	EFFECT_ENTITY_SPAWN_NIL = 0,

	EFFECT_ENTITY_SPAWN_WORLD = 1,
	EFFECT_ENTITY_SPAWN_IN_OBSTACLE = 2,

	EFFECT_ENTITY_SPAWN_MAX,
} EffectSpawn;

typedef enum EffectType {
	NIL_EFFECT = 0,

	// Positive Effects
	EFFECT_IMMORTAL_BOTTOM = 1,
	EFFECT_IMMORTAL_TOP = 2,
	EFFECT_EXTRA_HEALTH = 3,
	EFFECT_EXPAND_PLAYER = 4,
	EFFECT_SPEED_PLAYER = 5,
	EFFECT_MIRROR_PLAYER = 6,

	// Negative Effects
	EFFECT_SLOW_PLAYER = 20,
	EFFECT_GLIDE_PLAYER = 21,
	EFFECT_SLOW_PROJECTILE_PLAYER = 22,
	EFFECT_WEAKNESS_PLAYER = 23,

	MAX_EFFECT,
} EffectType;

typedef enum EntityType {
	ENTITY_NIL = 0,

	ENTITY_PLAYER = 1,
	ENTITY_PROJECTILE = 2,
	ENTITY_OBSTACLE = 3,
	ENTITY_EFFECT = 4,
	ENTITY_BOSS = 5,
	ENTITY_BEAM = 6,
	ENTITY_ROLLING_TEXT = 7,

	ENTITY_MAX,
} EntityType;