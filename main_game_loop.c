// -----------------------------------------------------------------------
// IMPORTS LOCAL FILES ONLY
// -----------------------------------------------------------------------
#include "include/easings.c"
#include "include/types.c"
#include "include/particlesystem.c"
#include "include/IndependentFunctions.c"

// -----------------------------------------------------------------------
// GLOBAL VARIABLES (!!!! WE USE #define and capital letters only !!!!)
// -----------------------------------------------------------------------
#define MAX_ENTITY_COUNT 1024
#define GRID_WIDTH 13
#define GRID_HEIGHT 13

// TODO: try to remove the need for global variables
int number_of_destroyed_obstacles = 0;
int number_of_shots_fired = 0;
int number_of_shots_missed = 0;
int number_of_power_ups = 0;
float projectile_speed = 500;
int number_of_hearts = 3;
float timer_power_up = 0;
Vector4 death_zone_bottom;
Vector4 death_zone_top;
bool debug_mode = false;
bool game_over = false;
bool is_power_up_active = false;
Vector2 mouse_position;
float64 delta_t;

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

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

typedef struct Entity {
	// --- Entity Attributes ---
	enum EntityType entitytype;
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	int health;
	bool is_valid;
	// --- Entity Type Below ---
	// Obstacle
	enum ObstacleType obstacle_type;
	float wave_time;
	float wave_time_beginning;
	float drop_interval;
	float drop_interval_timer;
	// Projectile
	int n_bounces;
	// Power UP
	enum PowerUpType power_up_type;
} Entity;

Entity entities[MAX_ENTITY_COUNT];

typedef struct ObstacleTuple {
	Entity* obstacle;
	int x;
	int y;
} ObstacleTuple;
ObstacleTuple obstacle_list[MAX_ENTITY_COUNT];
int obstacle_count = 0;

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

void particle_update(float64 delta_t) {
	for (int i = 0; i < ARRAY_COUNT(particles); i++) {
		Particle* p = &particles[i];
		if (!(p->flags & PARTICLE_FLAGS_valid)) {
			continue;
		}

		if (p->end_time && has_reached_end_time(os_get_elapsed_seconds(), p->end_time)) {
			particle_clear(p);
			continue;
		}

		if (p->flags & PARTICLE_FLAGS_fade_out_with_velocity && v2_length(p->velocity) < 0.01) {
			particle_clear(p);
		}

		if (p->flags & PARTICLE_FLAGS_gravity) {
			Vector2 gravity = v2(0, -98.2f);
			p->velocity = v2_add(p->velocity, v2_mulf(gravity, delta_t));
		}

		if (p->flags & PARTICLE_FLAGS_physics) {
			if (p->flags & PARTICLE_FLAGS_friction) {
				p->acceleration = v2_sub(p->acceleration, v2_mulf(p->velocity, p->friction));
			}
			p->velocity = v2_add(p->velocity, v2_mulf(p->acceleration, delta_t));
			Vector2 next_pos = v2_add(p->pos, v2_mulf(p->velocity, delta_t));
			p->acceleration = (Vector2){0};
			p->pos = next_pos;
		}
	}
}

void particle_render() {
	for (int i = 0; i < ARRAY_COUNT(particles); i++) {
		Particle* p = &particles[i];
		if (!(p->flags & PARTICLE_FLAGS_valid)) {
			continue;
		}

		Vector4 col = p->col;
		if (p->flags & PARTICLE_FLAGS_fade_out_with_velocity) {
			col.a *= float_alpha(fabsf(v2_length(p->velocity)), 0, p->fade_out_vel_range);
		}

		Vector2 draw_position = v2_sub(p->pos, v2_mulf(p->size, 0.5));
		draw_rect(draw_position, p->size, col);
	}
}

void particle_emit(Vector2 pos, Vector4 color, ParticleKind kind) {
	switch (kind) {
		case BOUNCE_PFX: {
			for (int i = 0; i < 4; i++) {
				Particle* p = particle_new();
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_friction | PARTICLE_FLAGS_fade_out_with_velocity;
				p->pos = pos;
				p->col = color;
				p->velocity = v2_normalize(v2(get_random_float32_in_range(-1, 1), get_random_float32_in_range(-1, 1)));
				p->velocity = v2_mulf(p->velocity, get_random_float32_in_range(200, 200));
				p->friction = 20.0f;
				p->fade_out_vel_range = 30.0f;
				p->size = v2(1, 1);
			}
		} break;
		case HIT_PFX: {
			for (int i = 0; i < 10; i++) {
				Particle* p = particle_new();
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_friction | PARTICLE_FLAGS_fade_out_with_velocity | PARTICLE_FLAGS_gravity;
				p->pos = pos;
				p->col = color;
				p->velocity = v2_normalize(v2(get_random_float32_in_range(-1, 1), get_random_float32_in_range(-1, 1)));
				p->velocity = v2_mulf(p->velocity, get_random_float32_in_range(100, 200));
				p->friction = get_random_float32_in_range(30.0f, 30.0f);
				p->fade_out_vel_range = 30.0f;
				p->end_time = os_get_elapsed_seconds() + get_random_float32_in_range(1.0f, 2.0f);
				p->size = v2(3, 3);
			}
		} break;
		case HARD_OBSTACLE_PFX: {
			for (int i = 0; i < 1; i++) {
				Particle* p = particle_new();
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_friction | PARTICLE_FLAGS_fade_out_with_velocity;
				p->pos = pos;
				p->col = color;
				p->velocity = v2_normalize(v2(get_random_float32_in_range(-1, 1), get_random_float32_in_range(-1, 1)));
				p->velocity = v2_mulf(p->velocity, get_random_float32_in_range(100, 200));
				p->friction = get_random_float32_in_range(30.0f, 30.0f);
				p->fade_out_vel_range = 30.0f;
				p->end_time = os_get_elapsed_seconds() + get_random_float32_in_range(1.0f, 2.0f);
				p->size = v2(3, 3);
			}
		} break;
		default: { log("Something went wrong with particle generation"); } break;
	}
}

void setup_player(Entity* entity) {
	entity->entitytype = PLAYER_ENTITY;

	entity->size = v2(50, 20);
	entity->position = v2(0, -300);
	entity->color = COLOR_WHITE;
}

void summon_projectile_player(Entity* entity, Entity* player) {
	entity->entitytype = PROJECTILE_ENTITY;

	int setup_y_offset = 10;
	entity->size = v2(10, 10);
	entity->position = v2_add(player->position, v2(0, setup_y_offset));
	entity->color = v4(0, 1, 0, 1); // Green color
	Vector2 normalized_velocity = v2_normalize(v2_sub(mouse_position, v2(player->position.x, player->position.y + setup_y_offset)));
	entity->velocity = v2_mulf(normalized_velocity, projectile_speed);
}

void summon_projectile_drop(Entity* entity, Entity* obstacle) {
	entity->entitytype = PROJECTILE_ENTITY;

	entity->size = v2(10, 10);
	entity->position = v2_add(obstacle->position, v2(0, -20));
	entity->color = v4(1, 0, 0, 0.5);
	entity->velocity = v2(0, -98.2);
}

void setup_power_up(Entity* entity) {
	entity->entitytype = POWERUP_ENTITY;

	int size = 25;
	entity->size = v2(size, size);
	entity->health = 1;
	number_of_power_ups++;

	float random_value = get_random_float64_in_range(0, 1);
	float n_powerups = 5;
	if (random_value < 1/n_powerups)
	{
		entity->power_up_type = EXPAND_POWER_UP;
		entity->color = COLOR_BLUE;
	} 
	else if (random_value < 2/n_powerups)
	{
		entity->power_up_type = IMMORTAL_BOTTOM_POWER_UP;
		entity->color = COLOR_GREEN;
	} 
	else if (random_value < 3/n_powerups)
	{
		entity->power_up_type = IMMORTAL_TOP_POWER_UP;
		entity->color = COLOR_YELLOW;
	} 
	else if (random_value < 4/n_powerups)
	{
		entity->power_up_type = HEALTH_POWER_UP;
		entity->color = COLOR_RED;
	}
	else
	{
		entity->power_up_type = SPEED_POWER_UP;
		entity->color = v4(0, 1, 1, 1);
	}
}

void setup_obstacle(Entity* entity, int x_index, int y_index, int n_rows) {
	entity->entitytype = OBSTACLE_ENTITY;

	int size = 20;
	int padding = 10;
	entity->size = v2(size, size);
	entity->position = v2(-180, -45);
	entity->position = v2_add(entity->position, v2(x_index*(size + padding), y_index*(size + padding)));
	
	// TODO: Make it more clear which block is harder
	float random_value = get_random_float64_in_range(0, 1);
	if (random_value <= 0.80 && y_index == 0) 
	{
		entity->obstacle_type = DROP_OBSTACLE;
		entity->health = 1;
		entity->drop_interval = get_random_float32_in_range(5.0f, 20.0f);
		entity->drop_interval_timer = 0;
	}
	else if (random_value <= 0.30) // 30% chance
	{
		// HARD obstacle
		entity->obstacle_type = HARD_OBSTACLE;
		entity->health = 2;
	} 
	else if (random_value <= 0.40)  // 10% chance
	{
		// BLOCK obstacle
		entity->obstacle_type = BLOCK_OBSTACLE;
		entity->health = 9999;
		entity->size = v2(30, 30);
	} 
	else
	{
		// BASE obstacle
		entity->obstacle_type = BASE_OBSTACLE;
		entity->health = 1;
		float red = 1 - (float)(x_index+1) / n_rows;
		float blue = (float)(x_index+1) / n_rows;
		entity->color = v4(red, 0, blue, 1);
	}
	
	obstacle_list[obstacle_count].obstacle = entity;
	obstacle_list[obstacle_count].x = x_index;
	obstacle_list[obstacle_count].y = y_index;
	obstacle_count++;
}

void projectile_bounce(Entity* projectile, Entity* obstacle) {
	projectile->n_bounces++;

	particle_emit(projectile->position, COLOR_WHITE, BOUNCE_PFX);

	Vector2 pos_diff = v2_sub(projectile->position, obstacle->position);
	if (fabsf(pos_diff.x) > fabsf(pos_diff.y)) 
	{
		projectile->position = v2_add(projectile->position, v2_mulf(projectile->velocity, -1 * delta_t));
		projectile->velocity = v2_mul(projectile->velocity, v2(-1,  1)); // Bounce x-axis
	} 
	else 
	{
		projectile->position = v2_add(projectile->position, v2_mulf(projectile->velocity, -1 * delta_t)); // go back 
		projectile->velocity = v2_mul(projectile->velocity, v2( 1, -1)); // Bounce y-axis
	}
}

void projectile_bounce_world(Entity* projectile) {
	projectile->n_bounces++;

	particle_emit(projectile->position, COLOR_WHITE, BOUNCE_PFX);

	projectile->velocity = v2_mul(projectile->velocity, v2(-1,  1)); // Bounce x-axis
}

void propagate_wave(Entity* hit_obstacle) {
    float wave_radius = 100.0f;  // Start with 0 radius and grow over time
	float wave_speed = 100.0f;

    Vector2 hit_position = hit_obstacle->position;
    // Iterate over all obstacles to propagate the wave
    for (int i = 0; i < obstacle_count; i++) {
        Entity* current_obstacle = obstacle_list[i].obstacle;
		
        if (current_obstacle != hit_obstacle) {  // Skip the hit obstacle itself
            float distance = v2_dist(hit_position, current_obstacle->position);

            // If within the wave radius, apply wave effect
            if (distance <= wave_radius) {
				float wave_delay = distance / wave_speed;
				float wave_time = max(0.0f, wave_delay);
                current_obstacle->wave_time = wave_time;
				current_obstacle->wave_time_beginning = wave_time;
            }
        }
    }
}

void apply_damage(Entity* obstacle, float damage) {
	obstacle->health -= damage;

	if (obstacle->health <= 0) {
		number_of_destroyed_obstacles ++;
		// Destroy the obstacle after its health is 0
		if (obstacle->entitytype == OBSTACLE_ENTITY) {
			propagate_wave(obstacle); 
		}
		entity_destroy(obstacle);
	}
}

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
void handle_projectile_collision(Entity* projectile, Entity* obstacle) {

    int damage = 1.0f; // This can be changes in the future

	if (obstacle->obstacle_type == HARD_OBSTACLE || obstacle->obstacle_type == BASE_OBSTACLE || obstacle->entitytype == POWERUP_ENTITY) { 
		apply_damage(obstacle, damage);
	}
	
	if (obstacle->obstacle_type == BLOCK_OBSTACLE) 
	{
		projectile_bounce(projectile, obstacle);
	} 
	else
	{
		entity_destroy(projectile);
	}
}

void apply_power_up(Entity* power_up, Entity* player) {
	switch(power_up->power_up_type) 
	{
		case(IMMORTAL_BOTTOM_POWER_UP): {
			death_zone_bottom = v4(0, 1, 0, 0.5);
			is_power_up_active = true;
			timer_power_up = 5.0f;
		} break;

		case(IMMORTAL_TOP_POWER_UP): {
			death_zone_top = v4(0, 1, 0, 0.5);
			is_power_up_active = true;
			timer_power_up = 5.0f;
		} break;

		case(EXPAND_POWER_UP): {
			player->size = v2_add(player->size, v2(10, 0));
			is_power_up_active = true;
			timer_power_up = 5.0f;
		} break;
		
		case(HEALTH_POWER_UP): {
			if (number_of_shots_missed > 0) {
				number_of_shots_missed--;
			}
		} break;

		case(SPEED_POWER_UP): {
			projectile_speed += 50;
		} break;

		default: { log("Something went wrong with powerups!"); } break;
	}
}

void update_power_up_timer(Entity* player, float delta_t) {
    // Kontrollera om power-upen är aktiv
    if (is_power_up_active) {
        timer_power_up -= delta_t;

        if (timer_power_up <= 0 && IMMORTAL_BOTTOM_POWER_UP) {
            // Återställ effekten när timern når 0
            death_zone_bottom = v4(1, 0, 0, 0.5);  // Återställ till standardfärg
            is_power_up_active = false;  // Deaktivera power-upen
        }
		if (timer_power_up <= 0 && IMMORTAL_TOP_POWER_UP) {
            death_zone_top = v4(1, 0, 0, 0.5); 
            is_power_up_active = false;  
        }
		if (timer_power_up <= 0 && EXPAND_POWER_UP) {
            player->size = v2_sub(player->size, v2(100, 0));
            is_power_up_active = false; 
		}
    }
}

// TODO: Better system for this
// Maybe have a world struct which holds all world variables?
void reset_values() {
	number_of_destroyed_obstacles = 0;
	number_of_shots_fired = 0;
	number_of_shots_missed = 0;
}

int entry(int argc, char **argv) {
	window.title = STR("Noel & Gustav - Pong Clone");
	window.point_width = 300;
	window.point_height = 500; 
	window.x = 200;
	window.y = 200;
	window.clear_color = COLOR_BLACK; // Background color
	death_zone_bottom = v4(1, 0, 0, 0.5);
	death_zone_top = v4(1, 0, 0, 0.5);
	draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

	float64 seconds_counter = 0.0;
	s32 frame_count = 0;
	float64 last_time = os_get_elapsed_seconds();
	
	Gfx_Font *font = load_font_from_disk(STR("C:/Windows/Fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");
	const u32 font_height = 48;

	Gfx_Image* heart_sprite = load_image_from_disk(STR("res/textures/heart.png"), get_heap_allocator());
	
	// Here we create the player entity object
	Entity* player = entity_create();
	setup_player(player);

	for (int x = 0; x < GRID_WIDTH; x++) { // x
		for (int y = 0; y < GRID_HEIGHT; y++) { // y
			if (get_random_float64_in_range(0, 1) <= 0.70) {
				Entity* entity = entity_create();
				setup_obstacle(entity, x, y, GRID_WIDTH);
			}
		}
	}

	// --------------------------------
	float random_position_power_up = get_random_int_in_range(-10, 10);
	while (!window.should_close) {
		reset_temporary_storage();

		// Time stuff
		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		// Uppdatera timern för power-up
        update_power_up_timer(player, delta_t);

		// Mouse Positions
		mouse_position = MOUSE_POSITION();

		// main code loop here --------------
		if (is_key_just_pressed(KEY_TAB)) 
		{
			consume_key_just_pressed(KEY_TAB);
			debug_mode = !debug_mode;  // Toggle debug_mode with a single line
		}

		if (is_key_just_pressed(KEY_ESCAPE) && game_over) {
			reset_values();
		}

		if (player->is_valid && !game_over)
		{
			// Hantera vänsterklick
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT) || is_key_just_pressed(KEY_SPACEBAR)) 
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				Entity* projectile = entity_create();
				summon_projectile_player(projectile, player);

				number_of_shots_fired++;
			}

			if (number_of_destroyed_obstacles % 5 == 0 && number_of_destroyed_obstacles != 0 && number_of_power_ups == 0){
				Entity* power_up = entity_create();
				setup_power_up(power_up);
			}

			Vector2 input_axis = v2(0, 0); // Create an empty 2-dim vector
			
			if ((is_key_down('A') || is_key_down(KEY_ARROW_LEFT)) && player->position.x > -window.point_width / 2 - player->size.x / 2)
			{
				input_axis.x -= 1.0;
			}

			if ((is_key_down('D') || is_key_down(KEY_ARROW_RIGHT)) && player->position.x <  window.point_width / 2 + player->size.x / 2)
			{
				input_axis.x += 1.0;
			}
			
			input_axis = v2_normalize(input_axis);
			player->position = v2_add(player->position, v2_mulf(input_axis, 500.0 * delta_t));
		}

		// Entity Loop for drawing and everything else
		int entity_counter = 0;
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) { // Here we loop through every entity
			Entity* entity = &entities[i];
			if (!entity->is_valid) continue;
			entity_counter++;

			entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));

			if (entity->entitytype == PROJECTILE_ENTITY) 
			{
				for (int j = 0; j < MAX_ENTITY_COUNT; j++) 
				{
					Entity* other_entity = &entities[j];
					if (entity == other_entity) continue;
					switch(other_entity->entitytype) {
						case(PLAYER_ENTITY):
						case(OBSTACLE_ENTITY): {
							if (circle_rect_collision(entity, other_entity)) 
							{
								//other_entity->wave_time = 0.0f;
								handle_projectile_collision(entity, other_entity);
								break;
							}
						} break;
						case(POWERUP_ENTITY): {
							if (circle_circle_collision(entity, other_entity)) 
							{
								particle_emit(other_entity->position, other_entity->color, HIT_PFX);
								apply_power_up(other_entity, player);
								handle_projectile_collision(entity, other_entity);
								number_of_power_ups--;
								break; // Exit after handling the first collision
							}
						} break;
						default: { break; }
					}
				}
				// If projectile bounce on the sidesb
				if (entity->position.x <=  -window.width / 2 || entity->position.x >=  window.width / 2)
				{
					projectile_bounce_world(entity);
				}

				// If projectile exit down 
				if (entity->position.y <= -window.height / 2)
				{
					if (death_zone_bottom.y == 1){
						entity_destroy(entity);
					}
					else 
					{
						number_of_shots_missed++;
						entity_destroy(entity);

					}
				}
				// If projectile exit up 
				if (entity->position.y >= window.height / 2)
				{
					if (death_zone_top.y == 1){
						entity_destroy(entity);
					}
					else 
					{
						number_of_shots_missed++;
						entity_destroy(entity);

					}
				}
			}
			
			{ // Draw The Entity
				float64 t = os_get_elapsed_seconds();
				if (entity->entitytype == PROJECTILE_ENTITY || entity->entitytype == POWERUP_ENTITY)
				{
					if(entity->entitytype == POWERUP_ENTITY)
					{
						entity->position = v2(window.width / 2 * sin(t + random_position_power_up), -100);
						float g = 0.2 * sin(10*t) + 0.8;
						float r = 0.2 * sin(10*t) + 0.8;
						float b = 0.2 * sin(10*t) + 0.8;
						switch(entity->power_up_type)
						{
							case(IMMORTAL_BOTTOM_POWER_UP):
								entity->color = v4(0, g, 0, 1);
								break;
							case(IMMORTAL_TOP_POWER_UP):
								entity->color = v4(r, g, 0, 1);
								break;
							case(EXPAND_POWER_UP):
								entity->color = v4(0, 0, b, 1);
								break;
							case(HEALTH_POWER_UP):
								entity->color = v4(r, 0, 0, 1);
								break;
							case(SPEED_POWER_UP):
								entity->color = v4(0, g, b, 1);
								break;
							default: break;
						}
					}
					
					Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
					draw_circle(draw_position, entity->size, entity->color);
				}

				if (entity->entitytype == PLAYER_ENTITY || entity->entitytype == OBSTACLE_ENTITY) 
				{
					if (entity->entitytype == OBSTACLE_ENTITY) 
					{
						switch(entity->obstacle_type) 
						{
							case(DROP_OBSTACLE): {
								entity->color = v4(1, 1, 1, 0.3);
							} break;
							case(HARD_OBSTACLE):
								float r = 0.5 * sin(t + 3*PI32) + 0.5;
								entity->color = v4(r, 0, 1, 1);
								break;
							case(BLOCK_OBSTACLE):
								float a = 0.3 * sin(2*t) + 0.7;
								entity->color = v4(0.2, 0.2, 0.2, a);
								break;
							default: { } break; 
						}
						if (entity->obstacle_type == DROP_OBSTACLE) {
							
							if (entity->drop_interval >= entity->drop_interval_timer) {
								entity->drop_interval_timer += delta_t;
								float drop_size = 10 * (entity->drop_interval_timer / entity->drop_interval);
								Vector2 draw_position = v2_sub(entity->position, v2_mulf(v2(drop_size, drop_size), 0.5));
								draw_circle(draw_position, v2(drop_size, drop_size), v4(1, 0, 0, 0.5));
							}
							else
							{
								entity->drop_interval_timer = 0.0f;
								Entity* drop_projectile = entity_create();
								summon_projectile_drop(drop_projectile, entity);
							}
						}
						if (entity->wave_time > 0.0f && entity->obstacle_type != BLOCK_OBSTACLE) {
							entity->wave_time -= delta_t;

							if (entity->wave_time < 0.0f) {
								entity->wave_time = 0.0f;
							}
							float normalized_wave_time = entity->wave_time / entity->wave_time_beginning;
							float wave_intensity = entity->wave_time / 0.1f;  // Intensity decreases with time
							
							// Vector4 diff_color = v4(1.0f, 0.0f, 0.0f, wave_intensity);  // Example: red wave effect
							float extra_size = 5.0f;

							float size_value = extra_size * easeOutBounce(normalized_wave_time);
							if (size_value >= extra_size * 0.5) {
								size_value = extra_size * easeOutBounce(entity->wave_time_beginning - normalized_wave_time);
							}
							Vector2 diff_size = v2_add(entity->size, v2(size_value, size_value));
							
							Vector2 draw_position = v2_sub(entity->position, v2_mulf(diff_size, 0.5));
							draw_rect(draw_position, diff_size, entity->color);
						}
						else
						{
							Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
							draw_rect(draw_position, entity->size, entity->color);
						}
						if (entity->obstacle_type == HARD_OBSTACLE) {
							if (get_random_float32_in_range(0, 1) <= 0.01) {
								particle_emit(entity->position, v4(1, 1, 1, 0.8), HARD_OBSTACLE_PFX);
							}
						}
					}
					else
					{
						Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
						draw_rect(draw_position, entity->size, entity->color);
					}
					
					
					
					Vector2 draw_position_2 = v2_sub(entity->position, v2_mulf(v2(5, 5), 0.5));
					if (debug_mode) { draw_circle(draw_position_2, v2(5, 5), COLOR_GREEN); }
				}
				
			}
		}

		
		if (debug_mode) {
			draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_destroyed_obstacles), font_height, v2(-window.width / 2, -window.height / 2 + 50), v2(0.7, 0.7), COLOR_RED);
			draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_shots_fired), font_height, v2(-window.width / 2, -window.height / 2 + 25), v2(0.7, 0.7), COLOR_GREEN);
		}
		
		// Check if game is over or not
		game_over = number_of_shots_missed >= number_of_hearts;

		if (game_over) {
			draw_text(font, sprint(get_temporary_allocator(), STR("GAME OVER"), number_of_shots_missed), font_height, v2(-window.width / 2, 0), v2(1.5, 1.5), COLOR_GREEN);
		}

		int heart_size = 30;
		int heart_padding = 10;
		for (int i = 0; i < max(number_of_hearts - number_of_shots_missed, 0); i++) {
			Vector2 heart_position = v2(window.width / 2 - (heart_size+heart_padding)*number_of_hearts, heart_size - window.height / 2);
			heart_position = v2_add(heart_position, v2((heart_size + heart_padding)*i, 0));
			draw_image(heart_sprite, heart_position, v2(heart_size, heart_size), v4(1, 1, 1, 1));
		}

		if (debug_mode) 
		{
			draw_line(player->position, mouse_position, 2.0f, COLOR_WHITE);
		}

		float wave = 15*(sin(now) + 1);
		draw_line(v2(-window.width / 2,  window.height / 2), v2(window.width / 2,  window.height/2), wave + 10.0f, death_zone_top);
		draw_line(v2(-window.width / 2, -window.height / 2), v2(window.width / 2, -window.height/2), wave + 10.0f, v4(0, (float)84/255, (float)119/225, 1));
		
		particle_update(delta_t);
		particle_render();
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