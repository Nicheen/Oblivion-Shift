// -----------------------------------------------------------------------
// Types (typedef enum)
// -----------------------------------------------------------------------

typedef enum TimedEventType {
	NIL_EVENTTYPE = 0,
	EVENT_COLOR_SWITCH = 1,
	BEAM_EVENT = 2,
	SNOW_EVENT = 3,
} TimedEventType;

typedef enum TimedEventWorldType {
	NIL_TIMERWORLDTYPE = 0,
	WORLD_TIMER = 1,
	ENTITY_TIMER = 2,
	STAGE_TIMER = 3,
} TimedEventWorldType;

typedef enum ParticleFlags {
    PARTICLE_FLAGS_valid = (1<<0),
	PARTICLE_FLAGS_physics = (1<<1),
	PARTICLE_FLAGS_friction = (1<<2),
	PARTICLE_FLAGS_fade_out_with_velocity = (1<<3),
	PARTICLE_FLAGS_gravity = (1<<4),
	// PARTICLE_FLAGS_bounce = (1<<4),
} ParticleFlags;

typedef enum ParticleKind {
	POWERUP_PFX,
	BOUNCE_PFX,
	HARD_OBSTACLE_PFX,
	SNOW_PFX,
} ParticleKind;

typedef enum ObstacleType {
	NIL_OBSTACLE = 0,

	BASE_OBSTACLE = 1,
	HARD_OBSTACLE = 2,
	BLOCK_OBSTACLE = 3,
	DROP_OBSTACLE = 4,
	POWERUP_OBSTACLE = 5,
	BEAM_OBSTACLE = 6,

	MAX_OBSTACLE,
} ObstacleType;
	
typedef enum PowerUpSpawn {
	POWER_UP_SPAWN_WORLD = 0,
	POWER_UP_SPAWN_IN_OBSTACLE = 1
} PowerUpSpawn;

typedef enum PowerUpType {
	NIL_POWER_UP = 0,

	IMMORTAL_BOTTOM_POWER_UP = 1,
	HEALTH_POWER_UP = 2,
	EXPAND_POWER_UP = 3,
	IMMORTAL_TOP_POWER_UP = 4,
	SPEED_POWER_UP = 5,

	MAX_POWER_UP,
} PowerUpType;

typedef enum DebuffType {
	NIL_DEBUFF = 0,

	DEBUFF_SLOW_PLAYER = 1,
	DEBUFF_GLIDE_PLAYER = 2,
	DEBUFF_SLOW_PROJECTILE = 3,
	DEBUFF_WEAKNESS = 4,


	MAX_DEBUFF,
} DebuffType;

typedef enum EntityType {
	NIL_ENTITY = 0,

	PLAYER_ENTITY = 1,
	PROJECTILE_ENTITY = 2,
	OBSTACLE_ENTITY = 3,
	POWERUP_ENTITY = 4,
	BOSS_ENTITY = 5,
	BEAM_ENTITY = 6,

	MAX_ENTITY,
} EntityType;