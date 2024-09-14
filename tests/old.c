

// -----------------------------------------------------------------------
// GLOBAL VARIABLES (!!!! WE USE #define and capital letters only !!!!)
// -----------------------------------------------------------------------
#define MAX_ENTITY_COUNT 1024
#define GRID_WIDTH 13
#define GRID_HEIGHT 13

float64 delta_t; // The only variable which can be initialized like this

// -----------------------------------------------------------------------
// HERE WE STORE GENERAL PURPOSE FUNCTIONS (MOSTLY MATH)
// -----------------------------------------------------------------------

inline float easeOutBounce(float x) {
	const float n1 = 7.5625;
	const float d1 = 2.75;

	if (x < 1 / d1) {
		return n1 * x * x;
	} else if (x < 2 / d1) {
		x -= 1.5 / d1;
		return n1 * x * x + 0.75;
	} else if (x < 2.5 / d1) {
		x -= 2.25 / d1;
    	return n1 * x * x + 0.9375;
	} else {
		x -= 2.625 / d1;
		return n1 * x * x + 0.984375;
	}
}

inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

Vector2 MOUSE_POSITION() {
	float mouseX = input_frame.mouse_x;
	float mouseY = input_frame.mouse_y;

	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;
	float window_w = window.width;
	float window_h = window.height;

	float ndc_x = (mouseX / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouseY / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	return (Vector2){world_pos.x, world_pos.y};
}

// -----------------------------------------------------------------------
// DIFFERENT TYPES
// -----------------------------------------------------------------------

typedef enum ObstacleType {
	NIL_OBSTACLE = 0,

	BASE_OBSTACLE = 1,
	HARD_OBSTACLE = 2,
	BLOCK_OBSTACLE = 3,

	MAX_OBSTACLE,
} ObstacleType;

typedef enum EntityType {
	NIL_ENTITY = 0,

	PLAYER_ENTITY = 1,
	PROJECTILE_ENTITY = 2,
	OBSTACLE_ENTITY = 3,
	POWERUP_ENTITY = 4,

	MAX_ENTITY,
} EntityType;

typedef enum PowerUpType {
	NIL_POWER_UP = 0,

	IMMORTAL_TOP_POWER_UP = 1,
	IMMORTAL_BOTTOM_POWER_UP = 2,
	HEALTH_POWER_UP = 3,
	EXPAND_POWER_UP = 4,
	SPEED_POWER_UP = 5,

	MAX_POWER_UP,
} PowerUpType;

// -----------------------------------------------------------------------
// ALL THE ENTITY STRUCTURES
// -----------------------------------------------------------------------

typedef struct World {
	float projectile_speed;
    float timer_power_up;
	int number_of_hearts;
    bool death_zone_bottom;
    bool death_zone_top;
    bool debug_mode;
    bool game_over;
    bool is_power_up_active;
} World;
World world = {500, 0, 3, true, true, false, false, false};

typedef struct Entity {
	// --- Common Attributes ---
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	int health;
	bool is_valid;
	bool is_rectangle;
	EntityType entity_type;
} Entity;

typedef struct Player {
    Entity entity;
    // --- Player-Specific Attributes ---
    int score;
	bool power_up_active;

	int number_of_destroyed_obstacles;
	int number_of_shots_fired;
	int number_of_shots_missed;
	int number_of_power_ups;
	
} Player;

typedef struct Obstacle {
    Entity entity;
    // --- Obstacle-Specific Attributes ---
    float wave_time;
    float wave_time_beginning;
    ObstacleType obstacle_type;
} Obstacle;

typedef struct ObstacleTuple {
	Obstacle obstacle;
	int count;
	int x;
	int y;
} ObstacleTuple;

typedef struct Projectile {
    Entity entity;
    // --- Projectile-Specific Attributes ---
    int number_of_bounces;
} Projectile;

typedef struct PowerUp {
	Entity entity;
	// --- Power-up-Specific Attributes ---
	bool initialize_timer;
	float timer_length;
	float timer;
	PowerUpType type;
} PowerUp;

Entity entities[MAX_ENTITY_COUNT]; // A list to hold all the entities
ObstacleTuple obstacle_list[MAX_ENTITY_COUNT]; // A list to hold all obstacles

// -----------------------------------------------------------------------
// Struct functions
// -----------------------------------------------------------------------

Entity* entity_create() {
	Entity* entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* existing_entity = &entities[i];
		if (!existing_entity->is_valid) {
			entity_found = existing_entity;
			break;
		}
	}
	assert(entity_found, "No more free entities!");
	entity_found->is_valid = true;
	return entity_found;
}

void entity_destroy(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

void projectile_destroy(Projectile* projectile) {
	entity_destroy(&projectile->entity);
	memset(projectile, 0, sizeof(Projectile));
}

void obstacle_destroy(Obstacle* obstacle) {
	entity_destroy(&obstacle->entity);
	memset(obstacle, 0, sizeof(Obstacle));
}

void powerup_destroy(PowerUp* powerup) {
	entity_destroy(&powerup->entity);
	memset(powerup, 0, sizeof(PowerUp));
}

// -----------------------------------------------------------------------
// Setup functions
// -----------------------------------------------------------------------

void setup_player(Player* player) {
	player->entity = *entity_create();
	player->entity.size = v2(50, 20);
	player->entity.color = COLOR_WHITE;

	player->entity.position = v2(0, -300);
	player->entity.is_rectangle = true;
	player->entity.entity_type = PLAYER_ENTITY;
}

void setup_obstacle(Obstacle* obstacle, int x, int y) {
	obstacle->entity = *entity_create();
	obstacle->entity.size = v2(20, 20);
	obstacle->entity.entity_type = OBSTACLE_ENTITY;
	obstacle->entity.is_rectangle = true;

	float x_offset = -180 + x * (1.5 * obstacle->entity.size.x);
    float y_offset = -45 + y * (1.5 * obstacle->entity.size.y);
    obstacle->entity.position = v2(x_offset, y_offset);

    // Initialize specific obstacle properties (e.g., wave time)
    obstacle->wave_time = 0.0f;
    obstacle->wave_time_beginning = 0.0f;

	float random_value = get_random_float64_in_range(0, 1);

	if (random_value <= 0.30) // 30% chance
	{
		obstacle->obstacle_type = HARD_OBSTACLE;
		obstacle->entity.health = 2;
	} 
	else if (random_value <= 0.40)  // 10% chance
	{
		obstacle->obstacle_type = BLOCK_OBSTACLE;
		obstacle->entity.health = 9999;
		obstacle->entity.size = v2(30, 30);
	} 
	else
	{
		obstacle->obstacle_type = BASE_OBSTACLE;
		obstacle->entity.health = 1;
		float red = 1 - (float)(x+1) / GRID_WIDTH;
		float blue = (float)(x+1) / GRID_WIDTH;
		obstacle->entity.color = v4(red, 0, blue, 1);
	}
}

// -----------------------------------------------------------------------
// Summon functions (more for temporary entities "after game start")
// -----------------------------------------------------------------------

void summon_projectile(Projectile* projectile, Vector2 summon_position) {
	projectile->entity = *entity_create();
	projectile->entity.size = v2(10, 10);
	projectile->entity.health = 1; // Projectile dies after 1 hit
	projectile->entity.color = COLOR_GREEN;
	projectile->entity.is_rectangle = false;
	projectile->entity.position = summon_position;
	Vector2 mouse_position = MOUSE_POSITION();
	projectile->entity.velocity = v2_mulf(v2_normalize(v2_sub(mouse_position, summon_position)), world.projectile_speed);
	projectile->entity.entity_type = PROJECTILE_ENTITY;
}

Vector4 get_color_for_power_up(PowerUpType power_up_type) {

	Vector4 colors[] = {COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW, COLOR_RED, v4(0, 1, 1, 1)};

    if (power_up_type < 0 || power_up_type >= MAX_POWER_UP) {
        // Handle invalid PowerUpType, possibly return a default color
        return COLOR_BLACK; // Define COLOR_BLACK if needed
    }
    return colors[power_up_type];
}

void summon_power_up(PowerUp* powerup) {
	powerup->entity = *entity_create();
    powerup->entity.size = v2(25, 25);
    powerup->entity.health = 1; // Powerups gets "popped" after 1 hit
	powerup->entity.is_rectangle = false;
	
    // Determine power-up type and color based on random value
    float random_value = get_random_float64_in_range(0, 1);
    int index = (int)(random_value * (MAX_POWER_UP - 1));

    powerup->type = (PowerUpType)(index + 1);
	powerup->entity.color = get_color_for_power_up(powerup->type);
}

// -----------------------------------------------------------------------
// Collision functions
// -----------------------------------------------------------------------

bool circle_rect_collision(Entity* circle, Entity* rect) {
	Vector2 distance = v2_sub(circle->position, rect->position);
	float dist_x = fabsf(distance.x);
	float dist_y = fabsf(distance.y);

	if (dist_x > (rect->size.x / 2.0f + circle->size.x / 2.0f)) { return false; }
	if (dist_y > (rect->size.y / 2.0f + circle->size.y / 2.0f)) { return false; }

	if (dist_x <= (rect->size.x / 2.0f)) { return true; }
	if (dist_y <= (rect->size.y / 2.0f)) { return true; }

	float dx = dist_x - rect->size.x / 2.0f;
	float dy = dist_y - rect->size.y / 2.0f;
	return (dx*dx+dy*dy <= circle->size.x*circle->size.y);
}

bool circle_circle_collision(Entity* circle1, Entity* circle2) {
    // Calculate the distance between the centers of the two circles
    Vector2 distance = v2_sub(circle1->position, circle2->position);
    float dist_squared = distance.x * distance.x + distance.y * distance.y;

    // Get the sum of the radii
    float radius_sum = (circle1->size.x / 2.0f) + (circle2->size.x / 2.0f);  // Assuming size.x is the diameter

    // Compare squared distance to squared radii sum (to avoid unnecessary sqrt calculation)
    return dist_squared <= radius_sum * radius_sum;
}

// -----------------------------------------------------------------------
// General functions
// -----------------------------------------------------------------------

void bounce(Entity* entity, Entity* other_entity) {
	Vector2 pos_diff = v2_sub(entity->position, other_entity->position);

	if (fabsf(pos_diff.x) > fabsf(pos_diff.y)) 
	{
		entity->position = v2_add(entity->position, v2_mulf(entity->velocity, -1 * delta_t));
		entity->velocity = v2_mul(entity->velocity, v2(-1,  1)); // Bounce x-axis
	} 
	else 
	{
		entity->position = v2_add(entity->position, v2_mulf(entity->velocity, -1 * delta_t));
		entity->velocity = v2_mul(entity->velocity, v2( 1, -1)); // Bounce y-axis
	}
}

void bounce_world(Entity* entity) {
	entity->position = v2_add(entity->position, v2_mulf(entity->velocity, -1 * delta_t));
	entity->velocity = v2_mul(entity->velocity, v2(-1,  1)); // Bounce x-axis
}

void propagate_wave(Obstacle* hit_obstacle) {
    float wave_radius = 100.0f;
	float wave_speed = 100.0f;

    Vector2 hit_position = hit_obstacle->entity.position;

    // Iterate over all obstacles to propagate the wave
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        Obstacle* obstacle = &obstacle_list[i].obstacle;
		
        if (obstacle != hit_obstacle) {  // Skip the hit obstacle itself
            float distance = v2_dist(hit_position, obstacle->entity.position);

            // If within the wave radius, apply wave effect
            if (distance <= wave_radius) {
				float wave_delay = distance / wave_speed;
				float wave_time = max(0.0f, wave_delay);
                obstacle->wave_time = wave_time;
				obstacle->wave_time_beginning = wave_time;
            }
        }
    }
}

void apply_damage(Entity* entity, float damage) {
	entity->health -= damage;

	if (entity->health <= 0) 
	{
		entity_destroy(entity);
	}
}

void apply_power_up(PowerUp* powerup, Player* player) {
	switch(powerup->type) 
	{
		case(HEALTH_POWER_UP):
			if (player->number_of_shots_missed > 0) {
				player->number_of_shots_missed--;
			}
			break;
		
		case(SPEED_POWER_UP):
			world.projectile_speed += 500;
			break;

		case(EXPAND_POWER_UP):
			powerup->initialize_timer = true;
			powerup->timer_length = 5.0f;
			// TODO: Implement a smoothing function (https://easings.net/#easeOutBounce)
			player->entity.size = v2_add(player->entity.size, v2(100, 0));
			break;
		
		case(IMMORTAL_BOTTOM_POWER_UP):
			powerup->initialize_timer = true;
			powerup->timer_length = 5.0f;
			world.death_zone_bottom = false;
			break;

		case(IMMORTAL_TOP_POWER_UP):
			powerup->initialize_timer = true;
			powerup->timer_length = 5.0f;
			world.death_zone_top = false;
			break;
		default: { break; }
	}
}

void handle_projectile_collision(Entity* projectile, Entity* entity) {
    int damage = 1.0f; // This can be changed in the future

	Obstacle* obstacle = (Obstacle*)entity; // Cast entity to Obstacle
	Player* player = (Player*)entity; // Cast entity to Player
	PowerUp* powerup = (PowerUp*)entity; // Cast entity to PowerUp

    switch (entity->entity_type) 
    {
        case OBSTACLE_ENTITY: 
        {
            switch(obstacle->obstacle_type) 
            {
                case BASE_OBSTACLE:
                case HARD_OBSTACLE:
                {
                    propagate_wave(obstacle);
                    apply_damage(entity, damage);
                } break;

                case BLOCK_OBSTACLE: 
                {
                    bounce(projectile, &obstacle->entity);
                } break;

                default: break;
            }
        } break;

        case PLAYER_ENTITY:
        {
            apply_damage(entity, damage); // Handle player-specific collision logic
        } break;

        case POWERUP_ENTITY:
        {
			apply_damage(entity, damage);
        	apply_power_up(powerup, player); // Assuming you pass the player here
        } break;

        default: break;
    }
}

void draw_shape(bool is_rect, Vector2 center_pos, Vector2 size, Vector4 col) {
	Vector2 draw_position = v2_sub(center_pos, v2_mulf(size, 0.5));
	if (is_rect)
	{
		draw_rect(draw_position, size, col);
	}
	else
	{
		draw_circle(draw_position, size, col);
	}
	if (world.debug_mode)
	{ 
		draw_circle(center_pos, v2(5, 5), COLOR_GREEN);
	}
}

void update_power_up_timer(PowerUp* powerup, Player* player) {

	if (powerup->initialize_timer) {
		powerup->timer = powerup->timer_length;
		powerup->initialize_timer = false;
	}

    if (player->power_up_active) {
        powerup->timer -= delta_t;
	}

	if (powerup->timer <= 0) {
		switch(powerup->type) {
			case(EXPAND_POWER_UP):
			// TODO: Implement a smoothing function (https://easings.net/#easeOutBounce)
			player->entity.size = v2_add(player->entity.size, v2(-100, 0));
			break;
		
			case(IMMORTAL_BOTTOM_POWER_UP):
				world.death_zone_bottom = true;
				break;

			case(IMMORTAL_TOP_POWER_UP):
				world.death_zone_top = true;
				break;

			default: { break; }
		}
	}
}

int entry(int argc, char **argv) {
	window.title = STR("BRICKED UP v0.0.1 (development build)"); // Set the window name
	window.point_width = 300;
	window.point_height = 500; 
	window.x = 200;
	window.y = 200;
	window.clear_color = COLOR_BLACK; // Set the background color

	// Set the camera view to (0, 0) in the middle of the window
	draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
	
	// -----------------------------------------------------------------------
	// Variable Creation
	// -----------------------------------------------------------------------

	float64 seconds_counter = 0.0;
	s32 frame_count = 0;
	float64 last_time = os_get_elapsed_seconds();
	
	// -----------------------------------------------------------------------
	// Load Images and Fonts
	// -----------------------------------------------------------------------

	Gfx_Font *font = load_font_from_disk(STR("C:/Windows/Fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading C:/Windows/Fonts/arial.ttf");
	const u32 font_height = 48;

	Gfx_Image* heart_sprite = load_image_from_disk(STR("res/textures/heart.png"), get_heap_allocator());
	assert(heart_sprite, "Failed to load ./res/textures/heart.png");

	// -----------------------------------------------------------------------
	// !!!!! Initialize Entities and Objects Into The World !!!!!
	// -----------------------------------------------------------------------

	Player* player;
	setup_player(player);

	int obstacle_count = 0;
	for (int x = 0; x < GRID_WIDTH; x++) {
		for (int y = 0; y < GRID_HEIGHT; y++) {
			if (get_random_float64_in_range(0, 1) <= 0.70) {
				Obstacle* obstacle = &obstacle_list[obstacle_count].obstacle;
				setup_obstacle(obstacle, x, y);

				obstacle_list[obstacle_count].obstacle = *obstacle;
				obstacle_list[obstacle_count].x = x;
				obstacle_list[obstacle_count].y = y;
				obstacle_count++;
			}
		}
	}

	// -----------------------------------------------------------------------
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!! GAME LOOP !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// -----------------------------------------------------------------------

	while (!window.should_close) {
		reset_temporary_storage();

		// --- Time stuff ---
		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		// -----------------------------------------------------------------------
		// Keyboard Inputs (Interrupts)
		// -----------------------------------------------------------------------

		if (is_key_just_pressed(KEY_TAB)) 
		{
			consume_key_just_pressed(KEY_TAB);
			world.debug_mode = !world.debug_mode;
		}

		if (!world.game_over)
		{
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) 
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				Projectile* projectile;
				summon_projectile(projectile, player->entity.position);
				player->number_of_shots_fired++;
			}

			if (player->number_of_destroyed_obstacles % 5 == 0 && player->number_of_destroyed_obstacles != 0 && player->number_of_power_ups == 0){
				PowerUp* powerup;
				summon_power_up(powerup);
			}

			Vector2 input_axis = v2(0, 0); // Create an empty 2-dim vector
			
			if ((is_key_down('A') || is_key_down(KEY_ARROW_LEFT)) && player->entity.position.x > -window.point_width / 2 - player->entity.size.x / 2)
			{
				input_axis.x -= 1.0;
			}

			if ((is_key_down('D') || is_key_down(KEY_ARROW_RIGHT)) && player->entity.position.x <  window.point_width / 2 + player->entity.size.x / 2)
			{
				input_axis.x += 1.0;
			}
			
			input_axis = v2_normalize(input_axis);
			player->entity.position = v2_add(player->entity.position, v2_mulf(input_axis, 500.0 * delta_t));
		}

		// -----------------------------------------------------------------------
		// Draw Loops for entities
		// -----------------------------------------------------------------------

		int entity_counter = 0;
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) { // Here we loop through every entity
			Entity* entity = &entities[i];
			Obstacle* obstacle = &obstacle_list[i].obstacle;

			float64 t = os_get_elapsed_seconds();

			if (!entity->is_valid) continue;
			entity_counter++;

			entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));

			switch (entity->entity_type) 
			{
				case(PROJECTILE_ENTITY): 
				{
					for (int j = 0; j < MAX_ENTITY_COUNT; j++) 
					{	
						Entity* other_entity = &entities[j];
						switch (entity->entity_type) 
						{
							case(OBSTACLE_ENTITY): 
							{
								if (circle_rect_collision(entity, other_entity))
								{
									handle_projectile_collision(entity, other_entity);
								} 
							} break;
							
							case(PLAYER_ENTITY):
							{
								if (circle_rect_collision(entity, other_entity))
								{
									handle_projectile_collision(entity, other_entity);
								} 
							} break;

							case(POWERUP_ENTITY):
							{
								if (circle_circle_collision(entity, other_entity)) 
								{
									PowerUp* powerup = (PowerUp*)entity;
									apply_power_up(powerup, player);
									handle_projectile_collision(entity, other_entity);
								}
							} break;
							default: { break; }
						}
					}
					// If projectile bounce on the sidesb
					if (entity->position.x <=  -window.width / 2 || entity->position.x >=  window.width / 2)
					{
						bounce_world(entity);
					}
					// If projectile exit down 
					if (entity->position.y <= -window.height / 2)
					{
						if (world.death_zone_bottom){
							entity_destroy(entity);
						}
						else 
						{
							player->number_of_shots_missed++;
							entity_destroy(entity);
						}
					}

					// If projectile exit up 
					if (entity->position.y >= window.height / 2)
					{
						player->number_of_shots_missed++;
						entity_destroy(entity);
					}
				} break;
				case(OBSTACLE_ENTITY): 
				{
					switch(obstacle->obstacle_type) 
					{
					case(BLOCK_OBSTACLE):
					{
						float a = 0.3 * sin(2*t) + 0.7;
						obstacle->entity.color = v4(0.2, 0.2, 0.2, a);
						draw_shape(obstacle->entity.is_rectangle, obstacle->entity.position, obstacle->entity.size, obstacle->entity.color);
					} break;
						
					case(HARD_OBSTACLE):
					{
						float r = 0.5 * sin(t + 3*PI32) + 0.5;
						obstacle->entity.color = v4(r, 0, 1, 1);
					}
					
					default:
					{
						if (obstacle->wave_time > 0.0f) 
						{
							Vector2 temp_size;
							Vector2 temp_position;
							Vector4 temp_color;

							obstacle->wave_time -= delta_t;
							if (obstacle->wave_time < 0.0f) {
								obstacle->wave_time = 0.0f;
							}

							float normalized_wave_time = obstacle->wave_time / obstacle->wave_time_beginning;
							float wave_intensity = obstacle->wave_time / 0.1f;  // Intensity decreases with time
							float extra_size = 5.0f;
							float size_value = extra_size * easeOutBounce(normalized_wave_time);
							if (size_value >= extra_size * 0.5) {
								size_value = extra_size * easeOutBounce(obstacle->wave_time_beginning - normalized_wave_time);
							}
							temp_size = v2_add(obstacle->entity.size, v2(size_value, size_value));
							draw_shape(obstacle->entity.is_rectangle, obstacle->entity.position, temp_size, obstacle->entity.color);
						}
						else
						{
							draw_shape(obstacle->entity.is_rectangle, obstacle->entity.position, obstacle->entity.size, obstacle->entity.color);
						}
					} break;	
					}
				}
				case(PLAYER_ENTITY):
				{
					draw_rect(entity->position, entity->size, entity->color);
				}
				default: {break;}
			}
		}

		// -----------------------------------------------------------------------
		// Draw UI and other stuff that are not entities
		// -----------------------------------------------------------------------

		if (world.debug_mode) {
			draw_text(font, sprint(get_temporary_allocator(), STR("%i"), player->number_of_destroyed_obstacles), font_height, v2(-window.width / 2, -window.height / 2 + 50), v2(0.7, 0.7), COLOR_RED);
			draw_text(font, sprint(get_temporary_allocator(), STR("%i"), player->number_of_shots_fired), font_height, v2(-window.width / 2, -window.height / 2 + 25), v2(0.7, 0.7), COLOR_GREEN);
		}
		
		// Check if game is over or not
		world.game_over = player->number_of_shots_missed >= world.number_of_hearts;

		if (world.game_over) {
			draw_text(font, sprint(get_temporary_allocator(), STR("GAME OVER"), player->number_of_shots_missed), font_height, v2(-window.width / 2, 0), v2(1.5, 1.5), COLOR_GREEN);
		}

		int heart_size = 30;
		int heart_padding = 10;
		for (int i = 0; i < max(world.number_of_hearts - player->number_of_shots_missed, 0); i++) {
			Vector2 heart_position = v2(window.width / 2 - (heart_size+heart_padding)*world.number_of_hearts, heart_size - window.height / 2);
			heart_position = v2_add(heart_position, v2((heart_size + heart_padding)*i, 0));
			draw_image(heart_sprite, heart_position, v2(heart_size, heart_size), v4(1, 1, 1, 1));
		}

		if (world.debug_mode) 
		{
			draw_line(player->entity.position, MOUSE_POSITION(), 2.0f, COLOR_WHITE);
		}

		float wave = 15*(sin(now) + 1);
		draw_line(v2(-window.width / 2,  window.height / 2), v2(window.width / 2,  window.height/2), wave + 10.0f, COLOR_GREEN);
		draw_line(v2(-window.width / 2, -window.height / 2), v2(window.width / 2, -window.height/2), wave + 10.0f, COLOR_GREEN);
		
		// main code loop here --------------a
		os_update(); 
		gfx_update();
		seconds_counter += delta_t;
		frame_count += 1;
		if (seconds_counter > 1.0) {
			log("fps: %i, n-entities: %i", frame_count, entity_counter);
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}