// -----------------------------------------------------------------------
//                      IMPORTS LOCAL FILES ONLY
// -----------------------------------------------------------------------
#include "include/GlobalVariables.c"
#include "include/IndependentFunctions.c"
#include "include/ExtraDrawFunctions.c"
#include "include/easings.c"
#include "include/Types.c"
#include "include/Structs.c"
#include "include/particlesystem.c"

// -----------------------------------------------------------------------
//                      Variable Global Variables
//            (means that they can change value during runtime,
//            if static variable use GlobalVariables.c instead)
// -----------------------------------------------------------------------
int number_of_destroyed_obstacles = 0;
int number_of_shots_fired = 0;
int number_of_shots_missed = 0;
int number_of_power_ups = 0;
int number_of_block_obstacles = 0;
float projectile_speed = 500;
int number_of_hearts = 3;
int current_stage_level = 0;
float timer_power_up = 0;
int obstacle_count = 0;

bool mercy_bottom;
bool mercy_top;
bool debug_mode = false;
bool game_over = false;
bool is_power_up_active = false;

//
// ---- Menu Booleans ---- 
//

bool is_main_menu_active = false;
bool is_settings_menu_active = false;
bool is_game_paused = false;

//
// ---- Initialize Important Variables ---- 
//

Vector2 mouse_position; // the current mouse position
float64 delta_t; // The difference in time between frames
World* world = 0; // Create an empty world to use for functions below

// -----------------------------------------------------------------------
//                  CREATE FUNCTIONS FOR ARRAY LOOKUP
// -----------------------------------------------------------------------
Entity* entity_create() {
	Entity* entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* existing_entity = &world->entities[i];
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

TimedEvent* timedevent_create(World* world) {
	TimedEvent* timedevent_found = 0;
	timedevent_found = alloc(get_heap_allocator(), sizeof(TimedEvent));
	memset(timedevent_found, 0, sizeof(TimedEvent));
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		TimedEvent* existing_timedevent = &world->timedevents[i];
		if (!existing_timedevent->is_valid) {
			timedevent_found = existing_timedevent;
			break;
		}
	}
	assert(timedevent_found, "No more free timedevent slots!");
	timedevent_found->is_valid = true;
	return timedevent_found;
}

void timedevent_destroy(TimedEvent* timedevent) {
	memset(timedevent, 0, sizeof(TimedEvent));
}

// TODO:
// Convert powerup and debuff to the same system as the ones above
//                            To create:
// PowerUp* powerup_create()
// void poweruo_destroy()
// Debuff* debuff_create()
// void debuff_destroy() 

// -----------------------------------------------------------------------
//                         TIMER FUNCTIONS
// -----------------------------------------------------------------------

bool timer_finished(TimedEvent* timed_event, float delta_t) {
    // Update the timer with the elapsed time
    if (!is_game_paused) timed_event->interval_timer += delta_t;

	// Progress calculation
	timed_event->progress = timed_event->interval_timer / timed_event->interval;

	// Ensure progress is clamped between 0 and 1
	timed_event->progress = fminf(fmaxf(timed_event->progress, 0.0f), 1.0f);

	// Check if the timer has exceeded the switch interval
	if (timed_event->interval_timer >= timed_event->interval) {

		if (timed_event->duration_timer < timed_event->duration)
		{
			if (!is_game_paused) timed_event->duration_timer += delta_t;
			return true;
		}
		else if (timed_event->duration_timer > timed_event->duration)
		{
			timed_event->duration_timer = 0;
		}
		
		timed_event->interval_timer -= timed_event->interval; // Reset the timer
		if (!(timed_event->counter == -1)) { timed_event->counter++; }
		return true;
	}
	else if (timed_event->interval_timer >= timed_event->interval) {
		return true;
	}

	return false;
}

TimedEvent* initialize_color_switch_event(World* world) {
	TimedEvent* te = timedevent_create(world);

	te->type = EVENT_COLOR_SWITCH;
	te->worldtype = WORLD_TIMER;
	te->interval = 4.0f;
	te->interval_timer = 0.0f;
	te->duration = 0.0f;
	te->progress = 0.0f;
	te->counter = 0;

	return te;
}

TimedEvent* initialize_beam_event(World* world) {
	TimedEvent* te = timedevent_create(world);

	te->type = BEAM_EVENT;
	te->worldtype = ENTITY_TIMER;
	te->interval = 10.0f;
	te->interval_timer = get_random_float32_in_range(0, 0.7*te->interval);
	te->duration = 1.0f;
	te->progress = 0.0f;
	te->counter = 0;

	return te;
}

TimedEvent* initialize_boss_movement_event(World* world) {
	TimedEvent* te = timedevent_create(world);

    te->type = BOSS_MOVEMENT_EVENT;
    te->worldtype = ENTITY_TIMER;
    te->interval = 4.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

TimedEvent* initialize_boss_attack_event(World* world) {
	TimedEvent* te = timedevent_create(world);

    te->type = BOSS_ATTACK_EVENT;
    te->worldtype = ENTITY_TIMER;
    te->interval = 2.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

// -----------------------------------------------------------------------
//                          PARTICLE FUNCTIONS
// -----------------------------------------------------------------------

int number_of_certain_particle(ParticleKind kind) {
	int n = 0;
	for (int i = 0; i < ARRAY_COUNT(particles); i++) {
		Particle* p = &particles[i];
		if (p->kind == kind) {
			n++;
		}
	}
	return n;
}

void remove_all_particle_type(ParticleKind kind) {
	for (int i = 0; i < ARRAY_COUNT(particles); i++) {
		Particle* p = &particles[i];
		if (p->kind == kind) {
			particle_clear(p);
		}
	}
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
			Vector2 gravity = v2(0, -40.2f);
			if (p->identifier == 0) {
				p->acceleration = gravity;
			}
			else
			{
				p->acceleration = v2_mulf(gravity, p->identifier);
			}
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

int particle_render() {
	int number_of_particles = 0;
	for (int i = 0; i < ARRAY_COUNT(particles); i++) {
		Particle* p = &particles[i];
		if (!(p->flags & PARTICLE_FLAGS_valid)) {
			continue;
		}
		number_of_particles++;

		Vector2 window_size = v2(window.width, window.height);
		if (is_position_outside_walls_and_bottom(p->pos, window_size)) {
			if (p->immortal) 
			{
				p->velocity = v2(get_random_float32_in_range(-10, 10), p->identifier*-20);
				p->pos = v2(get_random_float32_in_range(-window.width / 2, window.width / 2), get_random_float32_in_range(window.height / 2, 3*window.height));
			}
			else
			{
				particle_clear(p);
				continue;
			}
		} else if (is_position_outside_bounds(p->pos, window_size)) {
			continue;
		}


		Vector4 col = p->col;
		if (p->flags & PARTICLE_FLAGS_fade_out_with_velocity) {
			col.a *= float_alpha(fabsf(v2_length(p->velocity)), 0, p->fade_out_vel_range);
		}
		draw_centered_rect(p->pos, p->size, col);
	}
	return number_of_particles;
}

void particle_emit(Vector2 pos, Vector4 color, int n_particles, ParticleKind kind) {
	switch (kind) {
		case BOUNCE_PFX: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = BOUNCE_PFX;
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
		case POWERUP_PFX: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = POWERUP_PFX;
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_friction | PARTICLE_FLAGS_fade_out_with_velocity | PARTICLE_FLAGS_gravity;
				p->pos = v2(pos.x + get_random_int_in_range(-10, 10), pos.y + get_random_int_in_range(-10, 10));
				p->col = color;
				p->velocity = v2_normalize(v2(get_random_float32_in_range(-1, 1), get_random_float32_in_range(-1, 1)));
				p->velocity = v2_mulf(p->velocity, get_random_float32_in_range(100, 200));
				p->friction = get_random_float32_in_range(30.0f, 30.0f);
				p->fade_out_vel_range = 20.0f;
				p->end_time = os_get_elapsed_seconds() + get_random_float32_in_range(1.0f, 2.0f);
				p->size = v2(4, 4);
			}
		} break;
		case HARD_OBSTACLE_PFX: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = HARD_OBSTACLE_PFX;
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
		case SNOW_PFX: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = SNOW_PFX;
				float z = get_random_float32_in_range(0, 20);
				float a = float_alpha(z, 0, 20);
				
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_gravity;
				p->pos = v2(get_random_float32_in_range(-window.width / 2, window.width / 2), get_random_float32_in_range(window.height / 2, 3*window.height));
				p->col = v4(1.0, 1.0, 1.0, 1.0);
				p->velocity = v2(get_random_float32_in_range(-10, 10), a*-20);
				p->size = v2_mulf(v2(1, 1), a*3);
				p->immortal = true;
				p->identifier = a;
			}
		}
		default: { log("Something went wrong with particle generation"); } break;
	}
}

// -----------------------------------------------------------------------
//               SUMMON / SETUP / INITIALIZE FUNCTIONS
// -----------------------------------------------------------------------

Entity* setup_player_entity(Entity* entity) {
	entity->entitytype = PLAYER_ENTITY;

	entity->size = v2(50, 20);
	entity->position = v2(0, -300);
	entity->color = COLOR_WHITE;

	return entity;
}

Player* create_player() {
    // Allocate memory for the Player object
	Player* player = 0;
	player = alloc(get_heap_allocator(), sizeof(Player));
	memset(player, 0, sizeof(Player));
	player->immunity_timer = 0.0f; // Ingen immunitet vid skapandet
	player->is_immune = false; // Spelaren är inte immun

    // Initialize the Entity part of the Player
    player->entity = entity_create();
    
    // Set up the player's entity attributes
    player->entity = setup_player_entity(player->entity);  // Assuming setup_player handles the initial setup of the Entity
    
    // Additional player-specific attributes
    player->max_speed = 500.0f;  // Example max speed
    player->min_speed = 1.0f;   // Example min speed
	player->max_bounce = 200.0f;
	player->damp_bounce = 0.5;

	player->entity->acceleration = v2(2500.0f, 0.0f);
	player->entity->deceleration = v2(5000.0f, 0.0f);
    
    return player;
}

Boss* create_boss() {
	Boss* boss = 0;
	boss = alloc(get_heap_allocator(), sizeof(Boss));
	memset(boss, 0, sizeof(Boss));

	boss->entity = entity_create();
	boss->entity->entitytype = BOSS_ENTITY;
	boss->entity->health = 10;
	boss->entity->start_health = boss->entity->health;
	boss->entity->color = rgba(165, 242, 243, 255);
	boss->entity->size = v2(50, 50);
	boss->entity->start_size = boss->entity->size;

	TimedEvent* timed_event = initialize_boss_movement_event(world);
	TimedEvent* attack_event = initialize_boss_attack_event(world);
	boss->entity->timer = timed_event;
	boss->entity->second_timer = attack_event;

	return boss;
}

void summon_projectile_player(Entity* entity, Player* player) {
	entity->entitytype = PROJECTILE_ENTITY;

	int setup_y_offset = 20;
	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = v2_add(player->entity->position, v2(0, setup_y_offset));
	entity->color = v4(0, 1, 0, 1); // Green color
	Vector2 normalized_velocity = v2_normalize(v2_sub(mouse_position, v2(player->entity->position.x, player->entity->position.y + setup_y_offset)));
	entity->velocity = v2_mulf(normalized_velocity, projectile_speed);
}

void summon_projectile_drop(Entity* entity, Entity* obstacle) {
	entity->entitytype = PROJECTILE_ENTITY;

	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = v2_add(obstacle->position, v2(0, -20));
	entity->color = v4(1, 0, 0, 0.5);
	entity->velocity = v2(0, -50);
}

void setup_power_up(Entity* entity) {
	entity->entitytype = POWERUP_ENTITY;
	entity->power_up_spawn = POWER_UP_SPAWN_WORLD;

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
		entity->color = v4(1, 1, 0, 1);
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

void summon_power_up_drop(Entity* obstacle) {
	Entity* entity = entity_create();
	setup_power_up(entity);
	entity->position = obstacle->position;
	entity->power_up_spawn = POWER_UP_SPAWN_IN_OBSTACLE;
}

void setup_obstacle(Entity* entity, int x_index, int y_index) {
	entity->entitytype = OBSTACLE_ENTITY;

	int size = 21;
	int padding = 10;
	entity->size = v2(size, size);
	entity->grid_position = v2(x_index, y_index);
	entity->position = v2(-180, -45);
	entity->position = v2_add(entity->position, v2(x_index*(size + padding), y_index*(size + padding)));
	
	// TODO: Make it more clear which block is harder
	float random_value = get_random_float64_in_range(0, 1);

	// Check for Drop Obstacle (20% chance)
	if (random_value <= SPAWN_RATE_DROP_OBSTACLE) 
	{
		entity->obstacle_type = DROP_OBSTACLE;
		entity->health = 1;
		entity->drop_interval = get_random_float32_in_range(15.0f, 30.0f);
		entity->drop_duration_time = get_random_float32_in_range(3.0f, 5.0f);
		entity->drop_interval_timer = 0;
	} 

	// Check for Hard Obstacle (next 20% chance, i.e., 0.2 < random_value <= 0.4)
	else if (random_value <= SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE) 
	{
		entity->obstacle_type = HARD_OBSTACLE;
		entity->health = 2;
	} 
	// Check for Block Obstacle (next 10% chance, i.e., 0.4 < random_value <= 0.5)
	else if (random_value <= SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE + SPAWN_RATE_BLOCK_OBSTACLE) 
	{
		entity->obstacle_type = BLOCK_OBSTACLE;
		entity->health = 9999;
		entity->size = v2(30, 30);

		number_of_block_obstacles++;
	} 
	// If none of the above, spawn the Base Obstacle (remaining 50%)
	else if (random_value <= SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE + SPAWN_RATE_BLOCK_OBSTACLE + SPAWN_RATE_POWERUP_OBSTACLE)
	{
		entity->obstacle_type = POWERUP_OBSTACLE;
		entity->health = 1;
		entity->size = v2(28, 28);
	}
	else if(random_value <= SPAWN_RATE_BEAM_OBSTACLE + SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE + SPAWN_RATE_BLOCK_OBSTACLE + SPAWN_RATE_POWERUP_OBSTACLE) {
        entity->obstacle_type = BEAM_OBSTACLE;
		TimedEvent* timed_event = initialize_beam_event(world);
		entity->timer = timed_event;
        entity->health = 1;  // Strålen kan inte förstöras
        entity->drop_interval = get_random_float32_in_range(10.0f, 20.0f);  // Tid mellan strålar
        entity->drop_duration_time = 1.0f;  // Strålen varar i 1 sekund
        entity->drop_interval_timer = 0.0f;
    }
	else 
	{
		entity->obstacle_type = BASE_OBSTACLE;
		entity->health = 1;
		float red = 1 - (float)(x_index+1) / GRID_WIDTH;
		float blue = (float)(x_index+1) / GRID_WIDTH;
		entity->color = v4(red, 0, blue, 1);
	}

	world->obstacle_list[obstacle_count].obstacle = entity;
	world->obstacle_list[obstacle_count].x = x_index;
	world->obstacle_list[obstacle_count].y = y_index;
	obstacle_count++;
}

void setup_beam(Entity* beam_obstacle, Entity* beam) {
	beam->color = v4(1, 0, 0, 0.7);
	float beam_height = beam_obstacle->size.y + window.height;
	beam->size = v2(5, beam_height);
	float beam_y_position = beam_obstacle->position.y - beam_height / 2 - beam_obstacle->size.y / 2;
	beam->position = v2(beam_obstacle->position.x, beam_y_position);
}

// void summon_world() needs the be furthers down since it might use 
// some of the functions above when summoning the world.
void summon_world(float spawn_rate) {
	for (int x = 0; x < GRID_WIDTH; x++) { // x
		for (int y = 0; y < GRID_HEIGHT; y++) { // y
			if (get_random_float64_in_range(0, 1) <= spawn_rate) {
				Entity* entity = entity_create();
				setup_obstacle(entity, x, y);
			}
		}
	}
}

// -----------------------------------------------------------------------
//                     UNCATEGORIZED FUNCTIONS
// -----------------------------------------------------------------------

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

// Function to check clearance below a tile
bool check_clearance_below(ObstacleTuple obstacle_list[], int obstacle_count, int x, int y) {
    // Ensure the y+1 is within the matrix bounds
    if (y == 0) {
        return true; 
    }

    // Iterate through all rows below (x, y), i.e., (x, y-1) down to (x, 0)
    for (int i = y - 1; i >= 0; i--) {
        int found = 0;  // Flag to check if there's a tile at (x, i)
        
        // Check if there's an obstacle at (x, i)
        for (int j = 0; j < obstacle_count; j++) {
            if (obstacle_list[j].x == x && obstacle_list[j].y == i) {
                found = 1;  // We found a tile at (x, i)
                
                // If the obstacle is not clear (i.e., not NULL), return 0
                if (obstacle_list[j].obstacle != NULL && obstacle_list[j].obstacle->obstacle_type != NIL_OBSTACLE) {
                    return false;  // If the obstacle is not destroyed, return 0
                }
                break;  // Break the inner loop once we've found the tile
            }
        }
        
        // If no obstacle was found at (x, i), it means the tile is clear, so continue checking
    }

    // If we made it through the loop, all tiles below are clear
    return true;
}

void clean_world() {
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		TimedEvent* timer = &world->timedevents[i];
		if (entity->entitytype == OBSTACLE_ENTITY && entity->is_valid) {
			entity_destroy(entity);
		}
		if (entity->entitytype == BOSS_ENTITY && entity->is_valid) {
			entity_destroy(entity);
		}
		if (!(timer->worldtype == WORLD_TIMER) && timer->is_valid) {
			timedevent_destroy(timer);
		}
	}
	// Iterate over the obstacle list and destroy each obstacle
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        if (world->obstacle_list[i].obstacle != NULL) {
            entity_destroy(world->obstacle_list[i].obstacle);  // Destroy the obstacle
            world->obstacle_list[i].obstacle = NULL;           // Nullify the reference
        }
    }
	obstacle_count = 0;
	number_of_block_obstacles = 0;
}

void projectile_bounce(Entity* projectile, Entity* obstacle) {
	projectile->n_bounces++;
	if (projectile->n_bounces >= projectile->max_bounces) {
		entity_destroy(projectile);
		return;
	}
	particle_emit(projectile->position, COLOR_WHITE, 10, BOUNCE_PFX);

	Vector2 pos_diff = v2_sub(projectile->position, obstacle->position);
	if (fabsf(pos_diff.x) > fabsf(pos_diff.y)) 
	{
		projectile->position = v2_add(projectile->position, v2_mulf(projectile->velocity, -1 * delta_t));
		projectile->velocity = v2_mul(projectile->velocity, v2(-1,  1)); // Bounce x-axis0
	} 
	else 
	{
		projectile->position = v2_add(projectile->position, v2_mulf(projectile->velocity, -1 * delta_t)); // go back 
		projectile->velocity = v2_mul(projectile->velocity, v2( 1, -1)); // Bounce y-axis
	}
}

void projectile_bounce_world(Entity* projectile) {
	projectile->n_bounces++;
	if (projectile->n_bounces >= projectile->max_bounces) {
		entity_destroy(projectile);
		return;
	}
	particle_emit(projectile->position, COLOR_WHITE, 10, BOUNCE_PFX);
	
	projectile->position = v2_add(projectile->position, v2_mulf(projectile->velocity, -1 * delta_t)); // go back 
	projectile->velocity = v2_mul(projectile->velocity, v2(-1,  1)); // Bounce x-axis
}

void propagate_wave(Entity* hit_obstacle) {
    float wave_radius = 100.0f;  // Start with 0 radius and grow over time
	float wave_speed = 100.0f;

    Vector2 hit_position = hit_obstacle->position;
    // Iterate over all obstacles to propagate the wave
    for (int i = 0; i < obstacle_count; i++) {
        Entity* current_obstacle = world->obstacle_list[i].obstacle;
		
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

void apply_damage(Entity* entity, float damage) {
	entity->health -= damage;

	if (entity->health <= 0) {
		if (entity->entitytype == OBSTACLE_ENTITY) {
			number_of_destroyed_obstacles ++;
			propagate_wave(entity);

			if (entity->obstacle_type == POWERUP_OBSTACLE) {
				summon_power_up_drop(entity);
			} 

			// Get the grid position of the obstacle from the entity
			Vector2 position = entity->grid_position;
			int x = position.x;
			int y = position.y;

			// Find the corresponding tuple in world->obstacle_list and nullify the obstacle
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				if (world->obstacle_list[i].x == x && world->obstacle_list[i].y == y) {
					entity_destroy(world->obstacle_list[i].obstacle);  // Nullify the obstacle
					world->obstacle_list[i].x = 0;
					world->obstacle_list[i].y = 0;
					obstacle_count--;
					break;
				}
			}
		}
		else
		{
			entity_destroy(entity);
		}
	}
}

void play_random_blop_sound() {
    // Slumpa ett tal mellan 0 och 3
    int random_sound = rand() % 4;

    switch (random_sound) {
        case 0:
			play_one_audio_clip(STR("res/sound_effects/blop1.wav"));
            break;
        case 1:
            play_one_audio_clip(STR("res/sound_effects/blop2.wav"));
            break;
        case 2:
            play_one_audio_clip(STR("res/sound_effects/blop3.wav"));
            break;
        case 3:
            play_one_audio_clip(STR("res/sound_effects/blop4.wav"));
            break;
    }
}

bool boss_is_alive(World* world) {
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;

		if (entity->entitytype == BOSS_ENTITY) {
			return true;
		}
	}
	return false;
}

// -----------------------------------------------------------------------
//                      COLLISION ALGORITHMS
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

bool rect_rect_collision(Entity* rect1, Entity* rect2) {
    // Beräkna gränserna för båda rektanglarna
    float rect1_left = rect1->position.x - rect1->size.x / 2.0f;
    float rect1_right = rect1->position.x + rect1->size.x / 2.0f;
    float rect1_top = rect1->position.y - rect1->size.y / 2.0f;
    float rect1_bottom = rect1->position.y + rect1->size.y / 2.0f;

    float rect2_left = rect2->position.x - rect2->size.x / 2.0f;
    float rect2_right = rect2->position.x + rect2->size.x / 2.0f;
    float rect2_top = rect2->position.y - rect2->size.y / 2.0f;
    float rect2_bottom = rect2->position.y + rect2->size.y / 2.0f;

    // Kontrollera om rektanglarna överlappar
    if (rect1_left > rect2_right || rect1_right < rect2_left ||
        rect1_top > rect2_bottom || rect1_bottom < rect2_top) {
        return false; // Ingen överlappning
    }
    
    return true; // Överlappning
}

// -----------------------------------------------------------------------
//                   HANDLE COLLISION FUNCTIONS
// -----------------------------------------------------------------------

void handle_projectile_collision(Entity* projectile, Entity* obstacle) {

    int damage = 1.0f; // This can be changes in the future
	
	if (obstacle->obstacle_type == BLOCK_OBSTACLE) 
	{
		projectile_bounce(projectile, obstacle);
		play_one_audio_clip(STR("res/sound_effects/thud1.wav"));
	} 
	else
	{
		if (!(obstacle->entitytype == PLAYER_ENTITY)) apply_damage(obstacle, damage);
		entity_destroy(projectile);
		play_random_blop_sound();
	}
}

void handle_beam_collision(Entity* beam, Entity* player) {
	int damage = 1.0f; // This can be changes in the future
	number_of_shots_missed++;
}

// -----------------------------------------------------------------------
//                        POWERUP FUNCTIONS
// -----------------------------------------------------------------------

// TODO: Det här kommer inte funka. Varje powerup behöver en egen timer. Här delar
// alla powerups samma timer, så om du hinner få en powerup innan timern börjar om
// så tappar du inte din powerup!

void apply_power_up(Entity* power_up, Player* player) {
	switch(power_up->power_up_type) 
	{
		case(IMMORTAL_BOTTOM_POWER_UP): {
			mercy_bottom = true;
			is_power_up_active = true;
			timer_power_up = 5.0f;
		} break;

		case(IMMORTAL_TOP_POWER_UP): {
			mercy_top = true;
			is_power_up_active = true;
			timer_power_up = 5.0f;
		} break;

		case(EXPAND_POWER_UP): {
			player->entity->size = v2_add(player->entity->size, v2(30, 0));
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

void update_power_up_timer(Player* player, float delta_t) {
    // Kontrollera om power-upen är aktiv
    if (is_power_up_active) {
        timer_power_up -= delta_t;
		// Återställ effekten när timern når 0
        if (timer_power_up <= 0 && IMMORTAL_BOTTOM_POWER_UP) {
            mercy_bottom = false;  // Återställ till standardfärg
            is_power_up_active = false;  // Deaktivera power-upen
        }
		if (timer_power_up <= 0 && IMMORTAL_TOP_POWER_UP) {
            mercy_top = false; 
            is_power_up_active = false;  
        }
		if (timer_power_up <= 0 && EXPAND_POWER_UP) {
            player->entity->size = v2_sub(player->entity->size, v2(10, 0));
            is_power_up_active = false; 
		}
    }
}

// -----------------------------------------------------------------------
//                         PLAYER FUNCTIONS
// -----------------------------------------------------------------------

void update_player_debuffs(Player* player, float delta_t) {
    for (int i = 0; i < MAX_DEBUFF_COUNT; i++) {
        if (player->player_debuffs[i].is_active) {
            // Endast öka tid om det inte är en permanent debuff
            if (player->player_debuffs[i].duration > 0) {
                player->player_debuffs[i].elapsed_time += delta_t;

                // Kolla om debuffens tid är slut
                if (player->player_debuffs[i].elapsed_time >= player->player_debuffs[i].duration) {
                    player->player_debuffs[i].is_active = false;  // Inaktivera debuffen
                    player->player_debuffs[i].debuff_type = NIL_DEBUFF;  // Ingen debuff
                }
            }
        }
    }
}

void update_player_immunity(Player* player, float delta_t) {
    if (player->is_immune) {
        player->immunity_timer -= delta_t; // Minska immunitetstimer
        if (player->immunity_timer <= 0.0f) {
            player->is_immune = false; // Immunitetens tid är slut
            player->immunity_timer = 0.0f; // Återställ timern
        }
    }
}

void apply_debuff_to_player(Player* player, DebuffType debuff_type, float duration) {
    for (int i = 0; i < MAX_DEBUFF_COUNT; i++) {
        if (!player->player_debuffs[i].is_active) {
            player->player_debuffs[i].debuff_type = debuff_type;
            player->player_debuffs[i].is_active = true;
            player->player_debuffs[i].duration = duration;
            player->player_debuffs[i].elapsed_time = 0.0f;
            break;
        }
    }
}

void apply_debuff_effects(Player* player) {
    for (int i = 0; i < MAX_DEBUFF_COUNT; i++) {
        if (player->player_debuffs[i].is_active) {
            switch (player->player_debuffs[i].debuff_type) {
                case DEBUFF_SLOW_PLAYER:
                    player->max_speed *= 0.5f;  // Halvera spelarens hastighet om slow-debuffen är aktiv
                    break;
				
				case DEBUFF_SLOW_PROJECTILE: //Projectile är inte med i struct player...
                    break;

                case DEBUFF_WEAKNESS: //Tänkte ha att man skadar 50% av vad man tidigare gjorde men måste ändra health till float då
                    break;

                case NIL_DEBUFF:
                default:
                    break;
            }
        }
    }
}

void update_player_position(Player* player, float delta_t) {
    Vector2 input_axis = v2(0, 0);  // Skapar en tom 2D-vektor för inmatning
    bool moving = false;
    static int previous_input = -1;  // -1: ingen riktning, 0: vänster, 1: höger

    // Kontrollera om vänster knapp trycks ned
    if ((is_key_down('A') || is_key_down(KEY_ARROW_LEFT)) && player->entity->position.x > -PLAYABLE_WIDTH / 2 + player->entity->size.x / 2) {
        input_axis.x -= 1.0f;
        moving = true;

        // Om tidigare input var höger, decelerera först innan vi byter riktning
        if (previous_input == 1 && player->entity->velocity.x > 0) {
            player->entity->velocity.x -= player->entity->deceleration.x * delta_t;
            if (player->entity->velocity.x < 0) player->entity->velocity.x = 0;  // Förhindra negativ hastighet under inbromsning
            return;  // Vänta tills vi bromsat in helt
        }
        previous_input = 0;  // Uppdatera riktningen till vänster
    }

    // Kontrollera om höger knapp trycks ned
    if ((is_key_down('D') || is_key_down(KEY_ARROW_RIGHT)) && player->entity->position.x < PLAYABLE_WIDTH / 2 - player->entity->size.x / 2) {
        input_axis.x += 1.0f;
        moving = true;

        // Om tidigare input var vänster, decelerera först innan vi byter riktning
        if (previous_input == 0 && player->entity->velocity.x < 0) {
            player->entity->velocity.x += player->entity->deceleration.x * delta_t;
            if (player->entity->velocity.x > 0) player->entity->velocity.x = 0;  // Förhindra positiv hastighet under inbromsning
            return;  // Vänta tills vi bromsat in helt
        }
        previous_input = 1;  // Uppdatera riktningen till höger
    }

    // Om både vänster och höger trycks ned samtidigt
    if ((is_key_down('A') || is_key_down(KEY_ARROW_LEFT)) && (is_key_down('D') || is_key_down(KEY_ARROW_RIGHT))) {
        input_axis.x = 0;  // Om båda trycks, sluta röra dig
        moving = false;
        previous_input = -1;  // Ingen aktiv rörelse
    }

    // Normalisera input_axis för att säkerställa korrekt riktning
    input_axis = v2_normalize(input_axis);

    // Acceleration
    if (moving) {
        // Om vi rör oss, accelerera
        player->entity->velocity = v2_add(player->entity->velocity, v2_mulf(input_axis, player->entity->acceleration.x * delta_t));

        // Begränsa hastigheten till maxhastigheten
        if (v2_length(player->entity->velocity) > player->max_speed) {
            player->entity->velocity = v2_mulf(v2_normalize(player->entity->velocity), player->max_speed);
        }
    } else {
        // Deceleration om ingen knapp trycks ned
        if (v2_length(player->entity->velocity) > player->min_speed) {
            Vector2 decel_vector = v2_mulf(v2_normalize(player->entity->velocity), -player->entity->deceleration.x * delta_t);
            player->entity->velocity = v2_add(player->entity->velocity, decel_vector);

            // Om hastigheten närmar sig 0, stoppa helt
            if (v2_length(player->entity->velocity) < player->min_speed) {
                player->entity->velocity = v2(0, 0);
            }
        }
    }
}

void limit_player_position(Player* player, float delta_t){
    // Begränsa spelarens position inom spelområdet
    if (player->entity->position.x > PLAYABLE_WIDTH / 2 - player->entity->size.x / 2) {
        player->entity->position.x = PLAYABLE_WIDTH / 2 - player->entity->size.x / 2;

        // Om hastigheten är tillräckligt hög, studsa tillbaka
        if (fabs(player->entity->velocity.x) > player->max_bounce) {
            player->entity->velocity.x = -player->entity->velocity.x * player->damp_bounce;  // Reflektera och dämpa hastigheten
        } else {
            player->entity->velocity.x = 0;  // Stanna om vi träffar kanten med låg hastighet
        }
    }

    if (player->entity->position.x < -PLAYABLE_WIDTH / 2 + player->entity->size.x / 2) {
        player->entity->position.x = -PLAYABLE_WIDTH / 2 + player->entity->size.x / 2;

        // Om hastigheten är tillräckligt hög, studsa tillbaka
        if (fabs(player->entity->velocity.x) > player->max_bounce) {
            player->entity->velocity.x = -player->entity->velocity.x * player->damp_bounce;  // Reflektera och dämpa hastigheten
        } else {
            player->entity->velocity.x = 0;  // Stanna om vi träffar kanten med låg hastighet
        }
    }
}

// -----------------------------------------------------------------------
//                   DRAW FUNCTIONS FOR DRAW LOOP
// -----------------------------------------------------------------------

void draw_beam(Entity* entity, float delta_t) {
	
	if (timer_finished(entity->timer, delta_t)) {
		if (entity->child == NULL) {
			Entity* beam = entity_create();
			setup_beam(entity, beam);
			entity->child = beam;
		}
		else
		{
			float duration_progress = entity->timer->duration_timer / entity->timer->duration;
			entity->child->size.x = 0.5 * entity->size.x * sin(PI32*duration_progress);
		}
	} else {
		if (entity->child != NULL) {
			entity_destroy(entity->child);
			entity->child = NULL;
		}
	}
	   if (entity->child == NULL && entity->timer->interval_timer >= entity->timer->interval - 1.0f) {

        float warning_progress = (entity->timer->interval - entity->timer->interval_timer) / 1.0f;
        float warning_beam_size = (entity->size.x / 4.0f) * (1.0f - warning_progress);

        Vector2 warning_position = v2_sub(entity->position, v2(warning_beam_size / 2, entity->size.y / 2));
        Vector2 warning_beam_size_vec = v2(warning_beam_size, -window.height); 
        draw_rect(warning_position, warning_beam_size_vec, v4(0, 1, 0, 0.5)); 
    }
	if (entity->child != NULL) {
		draw_centered_rect(entity->child->position, entity->child->size, v4(1, 0, 0, 0.7));
    }
}

void draw_playable_area_borders() {
	draw_line(v2(-PLAYABLE_WIDTH / 2, -window.height / 2), v2(-PLAYABLE_WIDTH / 2, window.height / 2), 2, COLOR_WHITE);
	draw_line(v2(PLAYABLE_WIDTH / 2, -window.height / 2), v2(PLAYABLE_WIDTH / 2, window.height / 2), 2, COLOR_WHITE);
}

void draw_death_borders(TimedEvent* timedevent, float delta_t) {

	// Use uint32_t instead of s64
	uint32_t lava_colors[5] = {
		0xff2500ff,
		0xff6600ff,
		0xf2f217ff,
		0xea5c0fff,
		0xe56520ff
	};

	Vector4 rgba_colors[5];
	for (int i = 0; i < 5; i++) {
		rgba_colors[i] = hex_to_rgba(lava_colors[i]);
	}

	if (timer_finished(timedevent, delta_t)) {
		if (timedevent->counter == 4) {
			timedevent->counter = 0;
		}
	}

	int next_color_index = (timedevent->counter + 1) % 5;
	Vector4 color = v4_lerp(rgba_colors[timedevent->counter], rgba_colors[next_color_index], timedevent->progress);

	float wave = 5*(sin(os_get_elapsed_seconds()) + 3);	
	
	draw_line(v2(-window.width / 2,  window.height / 2), v2(window.width / 2,  window.height/2), wave, color);
	draw_line(v2(-window.width / 2, -window.height / 2), v2(window.width / 2, -window.height/2), wave, color);

}

void draw_hearts(int number_of_hearts, int number_of_shots_missed, Gfx_Image* heart_sprite) {
    int heart_size = 50;
    int heart_padding = 0;
    
    // Calculate the number of hearts to draw
    int hearts_to_draw = max(number_of_hearts - number_of_shots_missed, 0);

    for (int i = 0; i < hearts_to_draw; i++) {
        // Calculate the initial heart position
        Vector2 heart_position = v2(window.width / 2 - (heart_size + heart_padding) * number_of_hearts, heart_size - window.height / 2);
        
        // Adjust the position for each heart
        heart_position = v2_add(heart_position, v2((heart_size + heart_padding) * i, 0));
        
        // Draw the heart sprite at the calculated position
        draw_image(heart_sprite, heart_position, v2(heart_size, heart_size), v4(1, 1, 1, 1));
    }
}

bool draw_button(Gfx_Font* font, u32 font_height, string label, Vector2 pos, Vector2 size, bool enabled) {
    if (font == NULL) {
        return false; // Early return if invalid inputs
    }

    Vector4 color = v4(.45, .45, .45, 1);

    float L = pos.x;
    float R = L + size.x;
    float B = pos.y;
    float T = B + size.y;

    float mx = input_frame.mouse_x - window.width / 2;
    float my = input_frame.mouse_y - window.height / 2;

    bool pressed = false;

    if (mx >= L && mx < R && my >= B && my < T) {
        color = v4(.15, .15, .15, 1);
        if (is_key_down(MOUSE_BUTTON_LEFT)) {
            color = v4(.05, .05, .05, 1);
        }

        pressed = is_key_just_released(MOUSE_BUTTON_LEFT);
    }

    if (enabled) {
        color = v4_sub(color, v4(.2, .2, .2, 0));
    }
	
    draw_rect(pos, size, color); 

    Gfx_Text_Metrics m = measure_text(font, label, font_height, v2(1, 1));
    if (m.visual_size.x <= 0 || m.visual_size.y <= 0) {
        return false; // Handle invalid metrics
    }

    Vector2 bottom_left = v2_sub(pos, m.functional_pos_min);
    bottom_left.x += size.x / 2;
    bottom_left.x -= m.functional_size.x / 2;

    bottom_left.y += size.y / 2;
    bottom_left.y -= m.functional_size.y / 2;

    draw_text(font, label, font_height, bottom_left, v2(1, 1), COLOR_WHITE);

    return pressed;
}

Vector2 update_boss_velocity(Vector2 velocity, float t) {
    // Example of modifying the velocity based on a sine wave
    float velocity_amplitude = get_random_float32_in_range(10.0, 15.0); // Amplitude for velocity changes
    float new_velocity_x = velocity_amplitude * (sin(t) + 0.5f * sin(2.0f * t)); // Modify x velocity
    float new_velocity_y = velocity_amplitude * (sin(t + 0.5f) + 0.3f * sin(3.0f * t)); // Modify y velocity

    return v2(new_velocity_x, new_velocity_y); // Return updated velocity
}

void summon_icicle(Entity* entity, Vector2 spawn_pos) {
	entity->entitytype = PROJECTILE_ENTITY;

	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = spawn_pos;
	entity->color = rgba(200, 233, 233, 255);
	entity->velocity = v2(0, -200);
}

void update_boss(Entity* entity, float delta_t) {
	if (timer_finished(entity->second_timer, delta_t)) {
		Entity* p1 = entity_create();
		Entity* p2 = entity_create();
		summon_icicle(p1, v2_add(entity->position, v2(entity->size.x, 0)));
		summon_icicle(p2, v2_sub(entity->position, v2(entity->size.x, 0)));
	}
}

void draw_boss(Entity* entity, float t, float delta_t) {
    // Update the velocity based on the timer
    if (timer_finished(entity->timer, delta_t)) {
        entity->velocity = update_boss_velocity(entity->velocity, t);
    }
    // Amplitudes for the movement (lower values for more subtle movement)
    float amplitude_x = 5.0f; // Reduced amplitude for x-axis
    float amplitude_y = 10.0f;  // Reduced amplitude for y-axis

    // Size scaling using a sine wave for a 3D effect
    float size_scale = 1.0f + 0.5f * sin(t); // Scale between 0.5 and 1.5
    entity->size = v2_mulf(entity->start_size, size_scale); // Use start_size for scaling

    // Draw the boss with the updated position and scaled size
    draw_centered_rect(entity->position, entity->size, entity->color);
    draw_centered_rect(v2_add(entity->position, v2(entity->size.x, 0)), v2_mulf(entity->size, 0.3), entity->color);
    draw_centered_rect(v2_add(entity->position, v2(-entity->size.x, 0)), v2_mulf(entity->size, 0.3), entity->color);
}

void draw_boss_health_bar(World* world, Gfx_Font* font) {
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;
		
		if (entity->entitytype == BOSS_ENTITY) {
			float health_bar_height = 50;
			float health_bar_width = 150;
			float padding = 15;
			float margin = 5;
			float alpha = (float)entity->health / (float)entity->start_health;
			draw_rect(v2(-health_bar_width / 2 - margin, window.height / 2 - health_bar_height - padding - margin), v2(health_bar_width + 2*margin, health_bar_height + 2*margin), v4(0.5, 0.5, 0.5, 0.5));
			draw_rect(v2(-health_bar_width / 2, window.height / 2 - health_bar_height - padding), v2(health_bar_width*alpha, health_bar_height), v4(0.9, 0.0, 0.0, 0.7));
			string label = sprint(get_temporary_allocator(), STR("%i"), entity->health);
			u32 font_height = 48;
			Gfx_Text_Metrics m = measure_text(font, label, font_height, v2(1, 1));
			draw_text(font, label, font_height, v2(-health_bar_width / 2 + health_bar_width*alpha / 2 - m.visual_size.x / 2, window.height / 2 - health_bar_height - padding + 10), v2(1, 1), COLOR_WHITE);
		}
	}
}

void draw_main_menu(Gfx_Font* font_light, Gfx_Font* font_bold) {
	if (is_main_menu_active) {
		draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 0.5));
		
		// Create the label using sprint
		string label = sprint(get_temporary_allocator(), STR("Main Menu"));
		u32 font_height = 48;
		float button_size = 200;
		Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));

		draw_text(font_bold, label, font_height, v2(-m.visual_size.x / 2, 75), v2(1, 1), COLOR_WHITE);
		
		// Play Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Play")), v2(-button_size / 2, 0), v2(button_size, 50), true)) {
			//play_one_audio_clip(sprint(get_temporary_allocator(), STR("res/sound_effects/Button_Click.wav")));
			is_main_menu_active = false;  // Close menu
        	is_game_paused = false;       // Resume game
		}

		// Settings Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Settings")), v2(-button_size / 2, -50), v2(button_size, 50), true)) {
			//play_one_audio_clip(sprint(get_temporary_allocator(), STR("res/sound_effects/Button_Click.wav")));
			is_main_menu_active = false;      // Close main menu
			is_settings_menu_active = true;   // Open settings menu
		}

		// Quit Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Quit")), v2(-button_size / 2, -100), v2(button_size, 50), true)) {
			window.should_close = true;  // Quit game
		}
	}
}

void draw_settings_menu(Gfx_Font* font_light, Gfx_Font* font_bold) {
	if (is_settings_menu_active) {
		draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 0.5));

		// Create the label using sprint
		string label = sprint(get_temporary_allocator(), STR("Settings"));
		u32 font_height = 48;
		Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));
		draw_text(font_bold, label, font_height, v2(-m.visual_size.x / 2, 75), v2(1, 1), COLOR_WHITE);

		Vector2 button_size = v2(200, font_height);
		int y = 0;

		// Sound Settings Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Sound")), v2(-button_size.x / 2, y), button_size, true)) {
			//play_one_audio_clip(sprint(get_temporary_allocator(), STR("res/sound_effects/Button_Click.wav")));
		}
		y -= button_size.y;
		// Graphics Settings Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Graphics")), v2(-button_size.x / 2, y), button_size, true)) {
			//play_one_audio_clip(sprint(get_temporary_allocator(), STR("res/sound_effects/Button_Click.wav")));
		}
		y -= button_size.y;
		// Controls Settings Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Controls")), v2(-button_size.x / 2, y), button_size, true)) {
			//play_one_audio_clip(sprint(get_temporary_allocator(), STR("res/sound_effects/Button_Click.wav")));
		}
		y -= button_size.y;
		// Back Button to return to Main Menu
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Back")), v2(-button_size.x / 2, y), button_size, true)) {
			is_settings_menu_active = false;  // Close settings menu
			is_main_menu_active = true;       // Return to main menu
		}
	}
}

void display_timed_event_info(TimedEvent* te, int index, float font_height, Gfx_Font* font_light, float* y_offset) {
    // Only proceed if the TimedEvent is valid
    if (!te->is_valid) return;

    // Start the string with basic info
    string temp = sprintf(get_temporary_allocator(), "TimedEvent %d - Type: %d", index, te->type);

    // Append other parameters conditionally
    if (te->interval != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, Interval: %.2f"), temp, te->interval);
    }
    if (te->interval_timer != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, I-timer: %.2f"), temp, te->interval_timer);
    }
    if (te->duration != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, Duration: %.2f"), temp, te->duration);
    }
    if (te->duration_timer != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, D-timer: %.2f"), temp, te->duration_timer);
    }
    if (te->progress != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, Progress: %.2f"), temp, te->progress);
    }
    if (te->counter <= 0) {
        temp = sprint(get_temporary_allocator(), STR("%s, Counter: %d"), temp, te->counter);
    }

    // Display the final constructed string at the current y_offset
    draw_text(font_light, 
        temp, 
        font_height, 
        v2(-window.width / 2, window.height / 2 - 200 - *y_offset),  // Adjust y-position based on y_offset
        v2(0.4, 0.4), 
        COLOR_GREEN
    );

    // Increase the y_offset for the next valid TimedEvent
    *y_offset += 25;  // Increase this value to adjust the vertical spacing between events
}

void render_timed_events(World* world, float font_height, Gfx_Font* font_light) {
    float y_offset = 0;  // Initialize y_offset to zero
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        display_timed_event_info(&world->timedevents[i], i, font_height, font_light, &y_offset);
    }
}

// -----------------------------------------------------------------------
//                      STAGE HANDLER FUNCTION
// -----------------------------------------------------------------------

void stage_0_to_9() {
	float r = 0.0f;
	float g = 0.0f;
	float b = (float)current_stage_level / 20.0f;
	world->world_background = v4(r, g, b, 1.0f); // Background color
	summon_world(SPAWN_RATE_ALL_OBSTACLES);

	int n_snow_particles = number_of_certain_particle(SNOW_PFX);
	particle_emit(v2(0, 0), v4(0, 0, 0, 0), 1000*((float)current_stage_level / (float)10) - n_snow_particles, SNOW_PFX);
}

void stage_10_boss() {
	Boss* boss = create_boss();

	int n_snow_particles = number_of_certain_particle(SNOW_PFX);
	particle_emit(v2(0, 0), v4(0, 0, 0, 0), 1000*((float)current_stage_level / (float)10) - n_snow_particles, SNOW_PFX);
}

void stage_11_to_19() {
	remove_all_particle_type(SNOW_PFX);
	float stage_progress = current_stage_level - 10;
	float r = stage_progress / 20.0f;
	float g = 0.0f;
	float b = 0.0f;
	world->world_background = v4(r, g, b, 1.0f); // Background color
	summon_world(SPAWN_RATE_ALL_OBSTACLES);
}

// !!!! Here we do all the stage stuff, not in the game loop !!!!
// Level specific stuff happens here
void initialize_new_stage(World* world, int current_stage_level) {

	clean_world();

	if (current_stage_level <= 9) 
	{
		stage_0_to_9();
	} 
	else if (current_stage_level == 10) 
	{
		stage_10_boss();
	} 
	else if (current_stage_level <= 20) 
	{
		stage_11_to_19();
	} 
	else 
	{
		remove_all_particle_type(SNOW_PFX);
		summon_world(SPAWN_RATE_ALL_OBSTACLES);
	}
}

int entry(int argc, char **argv) {
	window.title = STR("Noel & Gustav - Pong Clone");
	window.point_width = 600;
	window.point_height = 500; 
	window.x = 200;
	window.y = 200;
	window.clear_color = COLOR_BLACK; // Background color
	//window.fullscreen = true;

	draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

	float64 seconds_counter = 0.0;
	s32 frame_count = 0;
	int latest_fps;
	int latest_entites;
	const u32 font_height = 48;
	float64 last_time = os_get_elapsed_seconds();

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	// Load in images and other stuff from disk
	Gfx_Font *font_light = load_font_from_disk(STR("./res/fonts/Abaddon Light.ttf"), get_heap_allocator());
	assert(font_light, "Failed loading './res/fonts/Abaddon Light.ttf'");

	Gfx_Font *font_bold = load_font_from_disk(STR("./res/fonts/Abaddon Bold.ttf"), get_heap_allocator());
	assert(font_light, "Failed loading './res/fonts/Abaddon Bold.ttf'");
	
	Gfx_Image* heart_sprite = load_image_from_disk(STR("res/textures/heart.png"), get_heap_allocator());
	assert(heart_sprite, "Failed loading 'res/textures/heart.png'");

	Gfx_Image* power_up_heart_sprite = load_image_from_disk(STR("res/textures/powerup_heart.png"), get_heap_allocator());
	assert(heart_sprite, "Failed loading 'res/textures/powerup_heart.png'");

	// Here we create the player object
	Player* player = create_player();
	
	summon_world(SPAWN_RATE_ALL_OBSTACLES);
	world->world_background = COLOR_BLACK;

	// -----------------------------------------------------------------------
	//                      WORLD TIMED EVENTS ARE MADE HERE
	// -----------------------------------------------------------------------
	TimedEvent* color_switch_event = initialize_color_switch_event(world);

	while (!window.should_close) {
		reset_temporary_storage();

		// Time stuff
		float64 now = os_get_elapsed_seconds();
		
		float64 delta_t = now - last_time;
		last_time = now;

		// Mouse Positions
		mouse_position = MOUSE_POSITION();

		draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), world->world_background);

		// -----------------------------------------------------------------------
		//                        HERE WE DO BUTTON INPUTS
		// -----------------------------------------------------------------------
		//
		//                   BUTTONS USED ARE DOCUMENTED BELOW:
		//
		// KEY_TAB           = to switch debug mode on/off
		// KEY_ESCAPE        = to open/close main menu
		// MOUSE_BUTTON_LEFT = summon projectile at player towards mouse
		// KEY_SPACEBAR      = ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		// 
		//
		//
		// ---- BUTTONS THAT ONLY WORK IN DEBUG MODE ----
		// 1-9 = Increment stage by that amount
		// R   = Reload the shader file

		if (is_key_just_pressed(KEY_TAB))
		{
			consume_key_just_pressed(KEY_TAB);
			debug_mode = !debug_mode;  // Toggle debug_mode with a single line
		}
		
		if (is_key_just_pressed(KEY_ESCAPE)) {
			consume_key_just_pressed(KEY_ESCAPE);

			if (is_settings_menu_active) {
				// Go back to main menu from settings
				is_settings_menu_active = false;
				is_main_menu_active = true;
			} else if (is_main_menu_active) {
				// If in the main menu, pressing ESC resumes the game
				is_main_menu_active = false;
				is_game_paused = false;
			} else {
				// If in the game, pressing ESC opens the main menu and pauses the game
				is_main_menu_active = true;
				is_game_paused = true;
			}
		}
		
		if (debug_mode) {
			for (char key = '1'; key <= '9'; key++) {
				if (is_key_just_pressed(key)) {
					consume_key_just_pressed(key);
					int level_increment = key - '0';  // Convert char to the corresponding integer
					current_stage_level += level_increment;
					initialize_new_stage(world, current_stage_level);
					window.clear_color = world->world_background;
					break;  // Exit the loop after processing one key
				}
			}
		}
	
		if (player->entity->is_valid && !game_over && !is_game_paused)
		{
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) 
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				Entity* projectile = entity_create();
				summon_projectile_player(projectile, player);

				number_of_shots_fired++;
			}

			else if (is_key_just_pressed(KEY_SPACEBAR)) 
			{
				consume_key_just_pressed(KEY_SPACEBAR);

				Entity* projectile = entity_create();
				summon_projectile_player(projectile, player);

				number_of_shots_fired++;
			}

			update_player_position(player, delta_t);
        	update_power_up_timer(player, delta_t);
		}

		// -----------------------------------------------------------------------
		//                      DRAW CENTER TEXT (STAGE LEVEL)
		// -----------------------------------------------------------------------

		{
			// Create the label using sprint
			string label = sprint(get_temporary_allocator(), STR("%i"), current_stage_level);
			u32 font_height = 48 + (current_stage_level*4);
			Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));
			draw_text(font_bold, label, font_height, v2(-m.visual_size.x / 2, 0), v2(1, 1), COLOR_WHITE);
		}

		// -----------------------------------------------------------------------
		//              Entity Loop for drawing and everything else
		// -----------------------------------------------------------------------

		int entity_counter = 0;
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* entity = &world->entities[i];
			if (!entity->is_valid) continue;
			entity_counter++;

			// Update positon from velocity of entity if game is not paused
			if (!(is_game_paused)) entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));
			
			if (entity->entitytype == PROJECTILE_ENTITY) 
			{
				bool collision = false;
				for (int j = 0; j < MAX_ENTITY_COUNT; j++) 
				{
					Entity* other_entity = &world->entities[j];
					if (entity == other_entity) continue;
					if (collision) break;
					switch(other_entity->entitytype) {
						case(PLAYER_ENTITY):
						case(BOSS_ENTITY):
						case(OBSTACLE_ENTITY):{
							if (circle_rect_collision(entity, other_entity)) 
							{
								handle_projectile_collision(entity, other_entity);
								collision = true;
							} 
						} break;

						case(PROJECTILE_ENTITY): {
							if (circle_circle_collision(entity, other_entity))
							{
								particle_emit(other_entity->position, other_entity->color, 4, POWERUP_PFX);
								handle_projectile_collision(entity, other_entity);
								collision = true;
							}
						} break;

						case(POWERUP_ENTITY): {
							if (circle_circle_collision(entity, other_entity)) 
							{
								particle_emit(other_entity->position, other_entity->color, 4, POWERUP_PFX);
								apply_power_up(other_entity, player);
								handle_projectile_collision(entity, other_entity);
								number_of_power_ups--;
								collision = true;
							}
						} break;
						default: { 
							continue; 
						} break;
					}
				}

				if (entity->position.x <=  -PLAYABLE_WIDTH / 2 || entity->position.x >=  PLAYABLE_WIDTH / 2)
				{
					projectile_bounce_world(entity);
					play_one_audio_clip(STR("res/sound_effects/vägg_thud.wav"));
				}

				if (entity->position.y <= -window.height / 2)
				{
					if (!mercy_bottom){
						number_of_shots_missed++;
						play_one_audio_clip(STR("res/sound_effects/Impact_021.wav"));
					}
					entity_destroy(entity);
				}

				if (entity->position.y >= window.height / 2)
				{
					if (!mercy_top) {
						number_of_shots_missed++;
						play_one_audio_clip(STR("res/sound_effects/Impact_021.wav"));
					}
					entity_destroy(entity);
				}
			}
			
			if (entity->obstacle_type == BEAM_OBSTACLE) 
			{
				draw_beam(entity, delta_t);
				if (entity->child != NULL) { // We have a beam
					if (rect_rect_collision(entity->child, player->entity)) {
						handle_beam_collision(entity->child, player->entity);
					}
				}
			}

			if (entity->entitytype == POWERUP_ENTITY)
			{
				if (entity->power_up_spawn == POWER_UP_SPAWN_WORLD) {
					float random_position_power_up = get_random_int_in_range(-PLAYABLE_WIDTH, PLAYABLE_WIDTH);
					entity->position = v2((entity->size.x - (PLAYABLE_WIDTH / 2)) * sin(now + random_position_power_up), -100);
				}
				
				float a = 0.2 * sin(7*now) + 0.8;
				Vector4 col = entity->color;
				entity->color = v4(col.x, col.y, col.z, a);

				draw_centered_circle(entity->position, v2_add(entity->size, v2(2, 2)), COLOR_BLACK);
				draw_centered_circle(entity->position, entity->size, entity->color);
			}

			if (entity->entitytype == PROJECTILE_ENTITY)
			{
				draw_centered_circle(entity->position, v2_add(entity->size, v2(2, 2)), COLOR_BLACK);
				draw_centered_circle(entity->position, entity->size, entity->color);
			}

			if (entity->entitytype == OBSTACLE_ENTITY) 
			{
				switch(entity->obstacle_type) 
				{
					case(DROP_OBSTACLE): {
						entity->color = v4(1, 1, 1, 0.3);
					} break;
					case(BEAM_OBSTACLE): {
						entity->color = v4(0, 1, 0, 0.3);
					} break;
					case(HARD_OBSTACLE):
						float r = 0.5 * sin(now + 3*PI32) + 0.5;
						entity->color = v4(r, 0, 1, 1);
						break;
					case(BLOCK_OBSTACLE):
						float a = 0.3 * sin(2*now) + 0.7;
						entity->color = v4(0.2, 0.2, 0.2, a);
						break;
					case(POWERUP_OBSTACLE):
						entity->color = v4(0, 0, 1, 1);
						break;
					default: { } break; 
				}
				if (entity->obstacle_type == DROP_OBSTACLE) {
					int x = entity->grid_position.x;
					int y = entity->grid_position.y;
					if (check_clearance_below(world->obstacle_list, obstacle_count, x, y)) {
						if (entity->drop_interval >= entity->drop_interval_timer) {
							if (!(is_game_paused)) entity->drop_interval_timer += delta_t;
							
							float drop_time_left = entity->drop_interval - entity->drop_interval_timer;

							if (drop_time_left < entity->drop_duration_time) {
								float drop_alpha = (float)(entity->drop_duration_time - drop_time_left) / entity->drop_duration_time;
								float drop_size = 10 * drop_alpha;
								Vector4 drop_color = v4_lerp(COLOR_GREEN, COLOR_RED, drop_alpha);
								draw_centered_circle(entity->position, v2(drop_size, drop_size), drop_color);
							}
						}
						else
						{
							entity->drop_interval = get_random_float32_in_range(15.0f, 30.0f);
							entity->drop_interval_timer = 0.0f;
							Entity* drop_projectile = entity_create();
							summon_projectile_drop(drop_projectile, entity);
						}
					}
				}

				if (entity->wave_time > 0.0f && entity->obstacle_type != BLOCK_OBSTACLE) {
					if (!(is_game_paused)) entity->wave_time -= delta_t;

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
					
					draw_rounded_centered_rect(entity->position, diff_size, entity->color, 0.1);
				}
				else
				{
					draw_rounded_centered_rect(entity->position, entity->size, entity->color, 0.1);
				}
			}

			if (entity->entitytype == PLAYER_ENTITY) {
				limit_player_position(player, delta_t);
				draw_centered_rect(entity->position, entity->size, entity->color);
			}

			if (entity->entitytype == BOSS_ENTITY) {
				update_boss(entity, delta_t);
				draw_boss(entity, now, delta_t);
			}
		}

		
		if (obstacle_count - number_of_block_obstacles <= 0 && !(boss_is_alive(world))) {
			current_stage_level++;
			initialize_new_stage(world, current_stage_level);
			window.clear_color = world->world_background;
		}

		if (!(is_game_paused)) { particle_update(delta_t); }
		
		int number_of_particles = particle_render();
		
		
		if (debug_mode) {
			draw_line(player->entity->position, mouse_position, 2.0f, v4(1, 1, 1, 0.5));
			draw_text(font_light, sprint(get_temporary_allocator(), STR("fps: %i"), latest_fps), font_height, v2(-window.width / 2, window.height / 2 - 50), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("entities: %i"), latest_entites), font_height, v2(-window.width / 2, window.height / 2 - 75), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("obstacles: %i, block: %i"), obstacle_count, number_of_block_obstacles), font_height, v2(-window.width / 2, window.height / 2 - 100), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("destroyed: %i"), number_of_destroyed_obstacles), font_height, v2(-window.width / 2, window.height / 2 - 125), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("projectiles: %i"), number_of_shots_fired), font_height, v2(-window.width / 2, window.height / 2 - 150), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("particles: %i"), number_of_particles), font_height, v2(-window.width / 2, window.height / 2 - 175), v2(0.4, 0.4), COLOR_GREEN);
			
			render_timed_events(world, font_height, font_light);
			
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) { // Here we loop through every entity
				Entity* entity = &world->entities[i];
				if (!entity->is_valid) continue;
				if (!(entity->entitytype == PLAYER_ENTITY) && !(entity->obstacle_type == BLOCK_OBSTACLE)) {
					draw_text(font_light, sprint(get_temporary_allocator(), STR("%i"), entity->health), font_height, v2_sub(entity->position, v2_mulf(entity->size, 0.5)), v2(0.2, 0.2), COLOR_GREEN);
				}	
			}

			
		}
		
		// Check if game is over or not
		game_over = number_of_shots_missed >= number_of_hearts;

		if (game_over) {
			//play_one_audio_clip(STR("res/sound_effects/Impact_038.wav"));
			current_stage_level = 0;
			number_of_shots_missed = 0;
			clean_world();
			remove_all_particle_type(SNOW_PFX);
			summon_world(SPAWN_RATE_ALL_OBSTACLES);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("GAME OVER\nSURVIVED %i STAGES"), current_stage_level), font_height, v2(0, 0), v2(1, 1), COLOR_WHITE);
		}

		draw_hearts(number_of_hearts, number_of_shots_missed, heart_sprite);

		draw_boss_health_bar(world, font_bold);

		draw_playable_area_borders();

		draw_death_borders(color_switch_event, delta_t);

		draw_settings_menu(font_light, font_bold);

		draw_main_menu(font_light, font_bold);

		
		os_update();
		gfx_update();

		seconds_counter += delta_t;
		frame_count += 1;
		if (seconds_counter > 1.0) {
			latest_fps = frame_count;
			latest_entites = entity_counter;
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}