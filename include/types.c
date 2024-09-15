typedef enum ParticleFlags {
    PARTICLE_FLAGS_valid = (1<<0),
	PARTICLE_FLAGS_physics = (1<<1),
	PARTICLE_FLAGS_friction = (1<<2),
	PARTICLE_FLAGS_fade_out_with_velocity = (1<<3),
	PARTICLE_FLAGS_gravity = (1<<4),
	// PARTICLE_FLAGS_bounce = (1<<4),
} ParticleFlags;

typedef enum ParticleKind {
	HIT_PFX,
	BOUNCE_PFX,
	HARD_OBSTACLE_PFX,
} ParticleKind;

typedef enum ObstacleType {
	NIL_OBSTACLE = 0,

	BASE_OBSTACLE = 1,
	HARD_OBSTACLE = 2,
	BLOCK_OBSTACLE = 3,

	MAX_OBSTACLE,
} ObstacleType;
	
typedef enum PowerUpType {
	NIL_POWER_UP = 0,

	IMMORTAL_BOTTOM_POWER_UP = 1,
	HEALTH_POWER_UP = 2,
	EXPAND_POWER_UP = 3,
	IMMORTAL_TOP_POWER_UP = 4,
	SPEED_POWER_UP = 5,

	MAX_POWER_UP,
} PowerUpType;

typedef enum EntityType {
	NIL_ENTITY = 0,

	PLAYER_ENTITY = 1,
	PROJECTILE_ENTITY = 2,
	OBSTACLE_ENTITY = 3,
	POWERUP_ENTITY = 4,

	MAX_ENTITY,
} EntityType;