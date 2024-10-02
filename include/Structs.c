// -----------------------------------------------------------------------
// Structs (typedef struct)
// -----------------------------------------------------------------------

typedef struct LightSource {
	Vector2 position;
	float intensity;
	float radius;
	Vector4 color;
	int type;
	Vector2 direction;
	float length;
} LightSource;

// BEWARE std140 packing:
// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
typedef struct Scene_Cbuffer {
	Vector2 mouse_pos_screen; // We use this to make a light around the mouse cursor
	Vector2 window_size; // We only use this to revert the Y in the shader because for some reason d3d11 inverts it.
	LightSource lights[MAX_LIGHTS];
	int light_count;
} Scene_Cbuffer;

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
} Entity;

typedef struct Effect{
	bool is_valid;
	TimedEvent* timer;
	float effect_duration;
	enum EffectType effect_type;
	enum EffectSpawn effect_spawn;
} Effect;

typedef struct Player{
	Entity* entity;
	float max_speed;
	float min_speed;
	float max_bounce;
	float damp_bounce;
	float immunity_timer;
    bool is_immune;
} Player;

typedef struct ObstacleTuple {
	Entity* obstacle;
	int x;
	int y;
} ObstacleTuple;

typedef struct World {
	Entity entities[MAX_ENTITY_COUNT];
	ObstacleTuple obstacle_list[MAX_ENTITY_COUNT];
    TimedEvent timedevents[MAX_ENTITY_COUNT];
	Effect effects[MAX_ENTITY_COUNT];

	Vector4 world_background;
} World;