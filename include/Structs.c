// -----------------------------------------------------------------------
// Structs (typedef struct)
// -----------------------------------------------------------------------

typedef struct My_Cbuffer {
	Vector2 mouse_pos_screen; // We use this to make a light around the mouse cursor
	Vector2 window_size; // We only use this to revert the Y in the shader because for some reason d3d11 inverts it.
} My_Cbuffer;

typedef struct TimedEvent {
    bool is_valid;
	TimedEventWorldType worldtype;
    TimedEventType type;     // Which timed event are we referencing
	float interval;          // Time before the event starts
    float interval_timer;    // Timer for the interval
    float duration;
    float duration_timer;          // Duration of the event
    float progress;          // Current progress of the event
    int counter;             // Count how many loops we have done (useful)
} TimedEvent;

typedef struct Entity {
    TimedEvent* timer;
	TimedEvent* second_timer;
	struct Entity* child;
	// --- Entity Attributes ---
	enum EntityType entitytype;
	Vector2 size;
	Vector2 start_size;
	Vector2 position;
	Vector2 velocity;
	Vector2 acceleration;
	Vector2 deceleration;
	Vector4 color;
	int start_health;
	int health;
	bool is_valid;
	// --- Entity Type Below ---
	// Obstacle
	enum ObstacleType obstacle_type;
	float wave_time;
	float wave_time_beginning;
	float drop_interval;
	float drop_interval_timer;
	float drop_duration_time;
	Vector2 grid_position;
	// Projectile
	int n_bounces;
	int max_bounces;
	enum PowerUpType power_up_type;
	enum PowerUpSpawn power_up_spawn;
} Entity;

typedef struct PowerUp{
	enum PowerUpType power_up_type;
	enum PowerUpSpawn power_up_spawn;
} PowerUp;

typedef struct Debuff{
	enum DebuffType debuff_type;
	bool is_active;
	float duration;
	float elapsed_time;
} Debuff;

typedef struct Player{
	Entity* entity;
	float max_speed;
	float min_speed;
	float max_bounce;
	float damp_bounce;
	Debuff player_debuffs[MAX_DEBUFF_COUNT];
	PowerUp player_powerups[MAX_POWERUP_COUNT];
	float immunity_timer;
    bool is_immune;
} Player;

typedef struct Boss {
	Entity* entity;
	// add more stuff here
} Boss;

typedef struct ObstacleTuple {
	Entity* obstacle;
	int x;
	int y;
} ObstacleTuple;

typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
	ObstacleTuple obstacle_list[MAX_ENTITY_COUNT];
    TimedEvent timedevents[MAX_ENTITY_COUNT];

	Debuff world_debuffs[MAX_DEBUFF_COUNT];
	PowerUp world_powerups[MAX_POWERUP_COUNT];

	Vector4 world_background;
} World;