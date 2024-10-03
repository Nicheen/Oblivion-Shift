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
int number_of_effects = 0;
int number_of_block_obstacles = 0;
int number_of_particles = 0;

float projectile_speed = 500;
int number_of_hearts = 3;
int current_stage_level = 0;
int obstacle_count = 0;
int light_count = 0;
int latest_fps = 0;
int latest_entites = 0;
const u32 font_height = 48;
LightSource lights[100];

Gfx_Font* font_light = NULL;
Gfx_Font* font_bold = NULL;
Gfx_Image* heart_sprite = NULL;
Gfx_Image* effect_heart_sprite = NULL;

bool debug_mode = false;
bool game_over = false;
bool use_shaders = true;  // Global variable to control shader usage

//
// ---- Starting Screen 
//
bool is_starting_screen_active = true;
bool is_game_running = false;

//
// ---- Menu Booleans ---- 
//

bool is_main_menu_active = false;
bool is_settings_menu_active = false;
bool is_game_paused = false;
bool is_graphics_settings_active = false;

//
// ---- Initialize Important Variables ---- 
//

Vector2 mouse_position; // the current mouse position
float64 delta_t; // The difference in time between frames
float64 now;
TimedEvent* color_switch_event = 0;
Player* player = 0;
World* world = 0; // Create an empty world to use for functions below

// -----------------------------------------------------------------------
//                  CREATE FUNCTIONS FOR ARRAY LOOKUP
// -----------------------------------------------------------------------
Entity* create_entity() {
	Entity* entity_found = 0;
	entity_found = alloc(get_heap_allocator(), sizeof(Entity));
	memset(entity_found, 0, sizeof(Entity));
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

void destroy_entity(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

TimedEvent* create_timedevent(World* world) {
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

void destroy_timedevent(TimedEvent* timedevent) {
	memset(timedevent, 0, sizeof(TimedEvent));
}

Effect* create_effect() {
	Effect* effect_found = 0;
	effect_found = alloc(get_heap_allocator(), sizeof(Effect));
	memset(effect_found, 0, sizeof(Effect));

	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Effect* existing_effect = &world->effects[i];
		if (!existing_effect->is_valid) {
			effect_found = existing_effect;
			break;
		}
	}
	assert(effect_found, "No more free effect slots!");
	effect_found->is_valid = true;

	TimedEvent* timer = 0;
	timer->is_valid = true;
	effect_found->timer = timer;
	
	return effect_found;
}

void destroy_effect(Effect* effect) {
	memset(effect, 0, sizeof(Effect));
}

// -----------------------------------------------------------------------
//                         TIMER FUNCTIONS
// -----------------------------------------------------------------------

bool timer_finished(TimedEvent* timed_event) {
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

		if (timed_event->counter != -1) timed_event->counter++; 
		
		return true;
	}
	else if (timed_event->interval_timer >= timed_event->interval) {
		return true;
	}

	return false;
}

TimedEvent* initialize_color_switch_event(World* world) {
	TimedEvent* te = create_timedevent(world);

	te->type = TIMED_EVENT_COLOR_SWITCH;
	te->worldtype = TIMED_EVENT_TYPE_WORLD;
	te->interval = 4.0f;
	te->interval_timer = 0.0f;
	te->duration = 0.0f;
	te->progress = 0.0f;
	te->counter = 0;

	return te;
}

TimedEvent* initialize_beam_event(World* world) {
	TimedEvent* te = create_timedevent(world);

	te->type = TIMED_EVENT_BEAM;
	te->worldtype = TIMED_EVENT_TYPE_ENTITY;
	te->interval = 10.0f;
	te->interval_timer = get_random_float32_in_range(0, 0.7*te->interval);
	te->duration = 1.0f;
	te->progress = 0.0f;
	te->counter = 0;

	return te;
}

TimedEvent* initialize_boss_movement_event(World* world) {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_BOSS_MOVEMENT;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = 4.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

TimedEvent* initialize_boss_attack_event(World* world) {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_BOSS_ATTACK;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = 2.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

TimedEvent* initialize_effect_event(World* world, float duration) {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_EFFECT;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = duration; // Interval for new random values
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

void create_light_source(Vector2 position, Vector4 color, float size, Scene_Cbuffer* scene_cbuffer) {
	LightSource light;
	light.position = v2_add(position, v2(window.width / 2, window.height / 2));
	light.intensity = 0.2f;
	light.radius = size;
	light.color = color;
	scene_cbuffer->lights[scene_cbuffer->light_count] = light;
	scene_cbuffer->light_count++;
}

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

void particle_update() {
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

int particle_render(Draw_Frame* frame) {
	number_of_particles = 0;
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

		draw_centered_in_frame_rect(p->pos, p->size, col, frame);
	}
	return number_of_particles;
}

void particle_emit(Vector2 pos, Vector4 color, int n_particles, ParticleKind kind) {
	switch (kind) {
		case PFX_BOUNCE: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_BOUNCE;
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
		case PFX_EFFECT: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_EFFECT;
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
		case PFX_HARD_OBSTACLE: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_HARD_OBSTACLE;
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
		case PFX_ASH: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_ASH;
				float z = get_random_float32_in_range(0, 20);
				float a = float_alpha(z, 0, 20);

				// Ge partikeln fysikegenskaper och rotation
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_gravity;
				
				// Placering och initial hastighet
				p->pos = v2(get_random_float32_in_range(-window.width / 2, window.width / 2), get_random_float32_in_range(window.height / 2, 3*window.height));
				p->col = v4(0.5, 0.3, 0.1, 1.0);  // Mörka askliknande färger
				p->velocity = v2(get_random_float32_in_range(-5, 5), a * -10); // Långsammare fall
				p->size = v2_mulf(v2(1, 1), a * 2);  // Mindre storlek än snö
				p->immortal = true;
				p->identifier = a;
			}
		} break;
		case PFX_LEAF: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_LEAF;
				float z = get_random_float32_in_range(0, 20);
				float a = float_alpha(z, 0, 20);

				// Ge partikeln fysikegenskaper och gravitation
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_gravity;
				
				// Placering och initial hastighet
				p->pos = v2(get_random_float32_in_range(-window.width / 2, window.width / 2), get_random_float32_in_range(window.height / 2, 3*window.height));
				p->col = v4(0.1 + (rand() / (float)RAND_MAX) * (0.3 - 0.1), 0.5 + (rand() / (float)RAND_MAX) * (0.8 - 0.5), 0.1 + (rand() / (float)RAND_MAX) * (0.2 - 0.1), 1.0);
				p->velocity = v2(get_random_float32_in_range(-10, 10), a * -15);  // Långsamt fallande
				p->size = v2_mulf(v2(2, 2), a * 3);  // Större, oregelbunden storlek
				p->immortal = true;
				p->identifier = a;
			}
		} break;
		case PFX_RAIN: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_RAIN;
				float z = get_random_float32_in_range(0, 20);
				float a = float_alpha(z, 0, 20);

				// Ge partikeln fysik och gravitation, men ingen rotation
				p->flags |= PARTICLE_FLAGS_physics | PARTICLE_FLAGS_gravity;
				
				// Placering och initial hastighet
				p->pos = v2(get_random_float32_in_range(-window.width / 2, window.width / 2), get_random_float32_in_range(window.height / 2, 3*window.height));
				p->col = v4(0.3, 0.3, 1.0, 0.5);  // Blå genomskinliga droppar
				p->velocity = v2(0, a * -300);  // Snabbare fall för regn
				p->size = v2_mulf(v2(0.5, 4), a * 4);  // Smala, långa droppar
				p->immortal = true;
				p->identifier = a;
			}
		} break;

		case PFX_WIND: { 			//DENNA ÄR RIKTIGT FUL...
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_WIND;

				// Generera ett slumptal för avstånd från höger sidan
				float z = get_random_float32_in_range(0, 20);
				float a = float_alpha(z, 0, 20);
				
				// Ge partikeln fysikegenskaper
				p->flags |= PARTICLE_FLAGS_physics; // Ingen gravitation för vind
				p->pos = v2(window.width / 2, get_random_float32_in_range(-window.height / 2, window.height / 2)); // Starta från höger sida
				p->col = v4(0.7, 0.7, 1.0, 0.5); // Ljusblå färg för vind
				p->velocity = v2(-50 * a, get_random_float32_in_range(-5, 5)); // Blåser åt vänster med mindre vertikal rörelse för mer stabilitet
				p->size = v2_mulf(v2(10, 1), a * 2); // Längre partikelstorlek för vind, med varierande storlek
				p->immortal = true; // Partiklarna ska finnas kvar
				p->identifier = a; // Identifiera partikeln
			}
		} break;
		case PFX_SNOW: {
			for (int i = 0; i < n_particles; i++) {
				Particle* p = particle_new();
				p->kind = PFX_SNOW;
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
		} break;

		default: { log("Something went wrong with particle generation"); } break;
	}
}

int calculate_particles_to_spawn(float alpha, int number_of_existing_particles, float max_particles) {
    // Ensure alpha is within the range [0, 1]
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    // Exponential growth curve (alpha^2)
    float target_particles = max_particles * (alpha * alpha);

    // Calculate the difference between target and existing particles
    int particles_to_spawn = (int)target_particles - number_of_existing_particles;

    // Ensure non-negative spawn count
    return (particles_to_spawn > 0) ? particles_to_spawn : 0;
}


// -----------------------------------------------------------------------
//               SUMMON / SETUP / INITIALIZE FUNCTIONS
// -----------------------------------------------------------------------

Entity* setup_player_entity(Entity* entity) {
	entity->entitytype = ENTITY_PLAYER;

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
    player->entity = create_entity();
    
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

void summon_projectile_player(Entity* entity, Player* player) {
	entity->entitytype = ENTITY_PROJECTILE;

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
	entity->entitytype = ENTITY_PROJECTILE;

	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = v2_add(obstacle->position, v2(0, -20));
	entity->color = v4(1, 0, 0, 0.5);
	entity->velocity = v2(0, -50);
}

void summon_icicle(Entity* entity, Vector2 spawn_pos) {
	entity->entitytype = ENTITY_PROJECTILE;

	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = spawn_pos;
	entity->color = rgba(200, 233, 233, 255);
	entity->velocity = v2(0, -200);
}

void setup_effect(Effect* effect) {

	float random_value = get_random_float64_in_range(0, 1);
	float n_effects = 5;

	effect->effect_duration = 4.0f;

	if (random_value < 1/n_effects)
	{
		effect->effect_type = EFFECT_EXPAND_PLAYER;
		effect->effect_spawn = EFFECT_ENTITY_SPAWN_IN_OBSTACLE;
		
	} 
	else if (random_value < 2/n_effects)
	{
		effect->effect_type = EFFECT_IMMORTAL_BOTTOM;
		effect->effect_spawn = EFFECT_ENTITY_SPAWN_IN_OBSTACLE;
	} 
	else if (random_value < 3/n_effects)
	{
		effect->effect_type = EFFECT_IMMORTAL_TOP;
		effect->effect_spawn = EFFECT_ENTITY_SPAWN_IN_OBSTACLE;
	} 
	else if (random_value < 4/n_effects)
	{
		effect->effect_type = EFFECT_EXTRA_HEALTH;
		effect->effect_spawn = EFFECT_ENTITY_SPAWN_IN_OBSTACLE;
	}
	else
	{
		effect->effect_type = EFFECT_SPEED_PLAYER;
		effect->effect_spawn = EFFECT_ENTITY_SPAWN_IN_OBSTACLE;
	}

	effect->timer = initialize_effect_event(world, effect->effect_duration);
}

void setup_boss_stage_10(Entity* entity) {
	entity->entitytype = ENTITY_BOSS;

	entity->health = 10;
	entity->start_health = entity->health;
	entity->color = rgba(165, 242, 243, 255);
	entity->size = v2(50, 50);
	entity->start_size = entity->size;

	entity->timer = initialize_boss_movement_event(world);
	entity->second_timer = initialize_boss_attack_event(world);
}

void setup_boss_stage_20(Entity* entity) {
	entity->entitytype = ENTITY_BOSS;

	entity->health = 10;
	entity->start_health = entity->health;
	entity->color = rgba(240, 100, 100, 255);
	entity->size = v2(50, 50);
	entity->start_size = entity->size;

	entity->timer = initialize_boss_movement_event(world);
	entity->second_timer = initialize_boss_attack_event(world);
}

void setup_obstacle(Entity* entity, int x_index, int y_index) {
	entity->entitytype = ENTITY_OBSTACLE;

	int size = 21;
	int padding = 10;
	entity->health = 1;
	entity->size = v2(size, size);
	entity->grid_position = v2(x_index, y_index);
	entity->position = v2(-180, -45);
	entity->position = v2_add(entity->position, v2(x_index*(size + padding), y_index*(size + padding)));

	// TODO: Make it more clear what each block does

	float random_value = get_random_float64_in_range(0, 1);

	// Check for Drop Obstacle (20% chance)
	if (random_value <= SPAWN_RATE_DROP_OBSTACLE) 
	{
		entity->obstacle_type = OBSTACLE_DROP;
		entity->drop_interval = get_random_float32_in_range(15.0f, 30.0f);
		entity->drop_duration_time = get_random_float32_in_range(3.0f, 5.0f);
		entity->drop_interval_timer = 0;
	} 

	// Check for Hard Obstacle (next 20% chance, i.e., 0.2 < random_value <= 0.4)
	else if (random_value <= SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE) 
	{
		entity->obstacle_type = OBSTACLE_HARD;
		entity->health = 2;
	} 
	// Check for Block Obstacle (next 10% chance, i.e., 0.4 < random_value <= 0.5)
	else if (random_value <= SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE + SPAWN_RATE_BLOCK_OBSTACLE) 
	{
		entity->obstacle_type = OBSTACLE_BLOCK;
		entity->health = 9999;
		entity->size = v2(30, 30);

		number_of_block_obstacles++;
	} 
	else if(random_value <= SPAWN_RATE_BEAM_OBSTACLE + SPAWN_RATE_DROP_OBSTACLE + SPAWN_RATE_HARD_OBSTACLE + SPAWN_RATE_BLOCK_OBSTACLE) {
        entity->obstacle_type = OBSTACLE_BEAM;
		TimedEvent* timed_event = initialize_beam_event(world);
		entity->timer = timed_event;
        entity->drop_interval = get_random_float32_in_range(10.0f, 20.0f);  // Tid mellan strålar
        entity->drop_duration_time = 1.0f;  // Strålen varar i 1 sekund
        entity->drop_interval_timer = 0.0f;
    }
	else 
	{
		entity->obstacle_type = OBSTACLE_BASE;
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

void setup_effect_entity(Entity* entity, Entity* obstacle) {
	entity->entitytype = ENTITY_EFFECT;

	entity->health = 1;
	entity->start_health = entity->health;
	entity->color = rgba(165, 242, 243, 255);
	entity->size = v2(20, 20);
	entity->start_size = entity->size;
	entity->position = obstacle->position;
}

// void summon_world() needs the be furthers down since it might use 
// some of the functions above when summoning the world.
void summon_world(float spawn_rate) {
	for (int x = 0; x < GRID_WIDTH; x++) { // x
		for (int y = 0; y < GRID_HEIGHT; y++) { // y
			if (get_random_float64_in_range(0, 1) <= spawn_rate) {
				Entity* entity = create_entity();
				setup_obstacle(entity, x, y);
			}
		}
	}
}

// -----------------------------------------------------------------------
//                        EFFECT FUNCTIONS
// -----------------------------------------------------------------------

// TODO: Varje effect behöver en egen timer

void apply_effect(Effect* effect) {
	switch(effect->effect_type) 
	{
		case(EFFECT_IMMORTAL_TOP): {
			break;
		} break;

		case(EFFECT_IMMORTAL_BOTTOM): {
			break;
		} break;

		case(EFFECT_EXPAND_PLAYER): {
			player->entity->size = v2_add(player->entity->size, v2(30, 0));
		} break;
		
		case(EFFECT_EXTRA_HEALTH): {
			if (number_of_shots_missed > 0) {
				number_of_shots_missed--;
			}
		} break;

		case(EFFECT_SPEED_PLAYER): {
			projectile_speed += 50;
		} break;

		default: { log("Something went wrong with effects!"); } break;
	}
}

string effect_pretty_text(int type) {
	switch(type) {
		case EFFECT_IMMORTAL_BOTTOM: return STR("Immortal Bottom");
		case EFFECT_IMMORTAL_TOP: return STR("Immortal Top");
		case EFFECT_EXTRA_HEALTH: return STR("Extra Health");
		case EFFECT_EXPAND_PLAYER: return STR("Expand Player");
		case EFFECT_SPEED_PLAYER: return STR("Speed Player");
		case EFFECT_MIRROR_PLAYER: return STR("Mirror Player");
		case EFFECT_SLOW_PLAYER: return STR("Slow Player");
		case EFFECT_GLIDE_PLAYER: return STR("Glide Player");
		case EFFECT_SLOW_PROJECTILE_PLAYER: return STR("Slow Projectile Player");
		case EFFECT_WEAKNESS_PLAYER: return STR("Weakness Player");
		default: return STR("ERROR");
	}
}

// -----------------------------------------------------------------------
//                     UNCATEGORIZED FUNCTIONS
// -----------------------------------------------------------------------

string view_mode_stringify(View_Mode vm) {
	switch (vm) {
		case VIEW_GAME_AFTER_POSTPROCESS:
			return STR("VIEW_GAME_AFTER_POSTPROCESS");
		case VIEW_GAME_BEFORE_POSTPROCESS:
			return STR("VIEW_GAME_BEFORE_POSTPROCESS");
		case VIEW_BLOOM_MAP:
			return STR("VIEW_BLOOM_MAP");
		default: return STR("");
	}
}

string get_timed_event_type_text(int timer_type) {
	switch (timer_type) {
		case TIMED_EVENT_COLOR_SWITCH: 
			return STR("Color Switch");
		case TIMED_EVENT_BEAM: 
			return STR("Laser Beam");
		case TIMED_EVENT_BOSS_MOVEMENT: 
			return STR("Boss Movement");
		case TIMED_EVENT_BOSS_ATTACK: 
			return STR("Boss Attack");
		case TIMED_EVENT_EFFECT: 
			return STR("Effect");
		default: 
			return STR("ERROR");
	}
}

string get_timed_event_world_type_text(int timer_world_type) {
	switch (timer_world_type) {
		case TIMED_EVENT_TYPE_WORLD: 
			return STR("World");
		case TIMED_EVENT_TYPE_ENTITY: 
			return STR("Entity");
		default: 
			return STR("ERROR");
	}
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
                if (obstacle_list[j].obstacle != NULL && obstacle_list[j].obstacle->obstacle_type != OBSTACLE_NIL) {
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
		if (entity->entitytype == ENTITY_OBSTACLE && entity->is_valid) {
			destroy_entity(entity);
		}
		if (entity->entitytype == ENTITY_BOSS && entity->is_valid) {
			destroy_timedevent(entity->timer);
			destroy_timedevent(entity->second_timer);
			destroy_entity(entity);
		}
		if (!(timer->worldtype == TIMED_EVENT_TYPE_WORLD) && timer->is_valid) {
			destroy_timedevent(timer);
		}
		if (world->obstacle_list[i].obstacle != NULL) {
            destroy_entity(world->obstacle_list[i].obstacle);  // Destroy the obstacle
            world->obstacle_list[i].obstacle = NULL;           // Nullify the reference
        }
	}

	obstacle_count = 0;
	number_of_block_obstacles = 0;
}

void projectile_bounce(Entity* projectile, Entity* obstacle) {
	projectile->n_bounces++;
	if (projectile->n_bounces >= projectile->max_bounces) {
		destroy_entity(projectile);
		return;
	}
	particle_emit(projectile->position, COLOR_WHITE, 10, PFX_BOUNCE);

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
		destroy_entity(projectile);
		return;
	}
	particle_emit(projectile->position, COLOR_WHITE, 10, PFX_BOUNCE);
	
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
		if (entity->entitytype == ENTITY_OBSTACLE) {
			number_of_destroyed_obstacles ++;
			//propagate_wave(entity);
			//float random_value_for_effect = get_random_float64_in_range(0, 1);

			Entity* effect_entity = create_entity();
			setup_effect_entity(effect_entity, entity);
			
			
			// Get the grid position of the obstacle from the entity
			Vector2 position = entity->grid_position;
			int x = position.x;
			int y = position.y;

			// Find the corresponding tuple in world->obstacle_list and nullify the obstacle
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				if (world->obstacle_list[i].x == x && world->obstacle_list[i].y == y) {
					destroy_entity(world->obstacle_list[i].obstacle);  // Nullify the obstacle
					world->obstacle_list[i].x = 0;
					world->obstacle_list[i].y = 0;
					obstacle_count--;
					break;
				}
			}
		}
		else
		{
			destroy_entity(entity);
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

		if (entity->entitytype == ENTITY_BOSS && entity->is_valid) {
			return true;
		}
	}
	return false;
}

void timed_event_info(TimedEvent* te, int index, float* y_offset) {
    // Only proceed if the TimedEvent is valid
    if (!te->is_valid) return;

    // Start the string with basic info
    string temp = sprintf(get_temporary_allocator(), "%s (%s)", get_timed_event_type_text(te->type), get_timed_event_world_type_text(te->worldtype));

    // Append other parameters conditionally
    if (te->interval != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, Interval: %.2f"), temp, te->interval);
    }
	if (te->duration != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, Duration: %.2f"), temp, te->duration);
    }
    if (te->interval != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, I-timer: %.2f"), temp, te->interval_timer);
    }
    if (te->duration != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, D-timer: %.2f"), temp, te->duration_timer);
    }
    if (te->interval != 0.0f) {
        temp = sprint(get_temporary_allocator(), STR("%s, Progress: %.2f"), temp, te->progress);
    }
    if (te->counter >= 0) {
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
	
	if (obstacle->obstacle_type == OBSTACLE_BLOCK) 
	{
		projectile_bounce(projectile, obstacle);
		play_one_audio_clip(STR("res/sound_effects/thud1.wav"));
	} 
	else
	{
		if (!(obstacle->entitytype == ENTITY_PLAYER)) { apply_damage(obstacle, damage); }
		destroy_entity(projectile);
		play_random_blop_sound();
	}
}

void handle_beam_collision(Entity* beam, Player* player) {
	// TODO: Implement a timer so player gets damaged slowly
	int damage = 1.0f; // This can be changes in the future
	number_of_shots_missed++;
	player->is_immune = true;
    player->immunity_timer = 1.0f;
}

// -----------------------------------------------------------------------
//                         PLAYER FUNCTIONS
// -----------------------------------------------------------------------
void update_player(Player* player) {
    // Hantera immunitet
    if (player->is_immune) {
        player->immunity_timer -= delta_t; // Minska timer
        if (player->immunity_timer <= 0.0f) {
            player->is_immune = false; // Spelaren är inte längre immun
        }
	if (player->immunity_timer <= 0.0f) {
            player->is_immune = false; // Spelaren är inte längre immun
        }
    }
    
}

void update_player_position(Player* player) {
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

void limit_player_position(Player* player){
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
//                   UPDATE FUNCTIONS FOR UPDATE LOOP
// -----------------------------------------------------------------------
void update_boss(Entity* entity) {
	if (timer_finished(entity->second_timer)) {
		Entity* p1 = create_entity();
		Entity* p2 = create_entity();
		summon_icicle(p1, v2_add(entity->position, v2(entity->size.x, 0)));
		summon_icicle(p2, v2_sub(entity->position, v2(entity->size.x, 0)));
	}
}

Vector2 update_boss_velocity(Vector2 velocity) {
    // Example of modifying the velocity based on a sine wave
    float velocity_amplitude = get_random_float32_in_range(10.0, 15.0); // Amplitude for velocity changes
    float new_velocity_x = velocity_amplitude * (sin(now) + 0.5f * sin(2.0f * now)); // Modify x velocity
    float new_velocity_y = velocity_amplitude * (sin(now + 0.5f) + 0.3f * sin(3.0f * now)); // Modify y velocity

    return v2(new_velocity_x, new_velocity_y); // Return updated velocity
}

void update_wave_effect(Entity* entity) {
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

	entity->size = v2_add(entity->start_size, v2(size_value, size_value));
}

// -----------------------------------------------------------------------
//                   DRAW FUNCTIONS FOR DRAW LOOP
// -----------------------------------------------------------------------

void draw_beam(Entity* entity, Draw_Frame* frame) {
	
	if (timer_finished(entity->timer)) {
		if (entity->child == NULL) {
			Entity* beam = create_entity();
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
			destroy_entity(entity->child);
			entity->child = NULL;
		}
	}
	   if (entity->child == NULL && entity->timer->interval_timer >= entity->timer->interval - 1.0f) {

        float warning_progress = (entity->timer->interval - entity->timer->interval_timer) / 1.0f;
        float warning_beam_size = (entity->size.x / 4.0f) * (1.0f - warning_progress);

        Vector2 warning_position = v2_sub(entity->position, v2(warning_beam_size / 2, entity->size.y / 2));
        Vector2 warning_beam_size_vec = v2(warning_beam_size, -window.height); 
        draw_rect_in_frame(warning_position, warning_beam_size_vec, v4(0, 1, 0, 0.5), frame); 
    }
	if (entity->child != NULL) {
		draw_centered_in_frame_rect(entity->child->position, entity->child->size, v4(1, 0, 0, 0.7), frame);
    }
}

void draw_playable_area_borders(Draw_Frame* frame) {
	draw_line_in_frame(v2(-PLAYABLE_WIDTH / 2, -window.height / 2), v2(-PLAYABLE_WIDTH / 2, window.height / 2), 2, COLOR_WHITE, frame);
	draw_line_in_frame(v2(PLAYABLE_WIDTH / 2, -window.height / 2), v2(PLAYABLE_WIDTH / 2, window.height / 2), 2, COLOR_WHITE, frame);
}

void draw_death_borders(TimedEvent* timedevent, Draw_Frame* frame) {

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

	if (timer_finished(timedevent)) {
		if (timedevent->counter == 4) {
			timedevent->counter = 0;
		}
	}

	int next_color_index = (timedevent->counter + 1) % 5;
	Vector4 color = v4_lerp(rgba_colors[timedevent->counter], rgba_colors[next_color_index], timedevent->progress);

	float wave = 5*(sin(os_get_elapsed_seconds()) + 3);	
	
	draw_line_in_frame(v2(-window.width / 2,  window.height / 2), v2(window.width / 2,  window.height/2), wave, color, frame);
	draw_line_in_frame(v2(-window.width / 2, -window.height / 2), v2(window.width / 2, -window.height/2), wave, color, frame);

}

void draw_hearts(Draw_Frame* frame) {
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
        draw_image_in_frame(heart_sprite, heart_position, v2(heart_size, heart_size), v4(1, 1, 1, 1), frame);
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

void draw_boss(Entity* entity, Draw_Frame* frame) {
    // Update the velocity based on the timer
    if (timer_finished(entity->timer)) {
        entity->velocity = update_boss_velocity(entity->velocity);
    }
    // Amplitudes for the movement (lower values for more subtle movement)
    float amplitude_x = 5.0f; // Reduced amplitude for x-axis
    float amplitude_y = 10.0f;  // Reduced amplitude for y-axis

    // Size scaling using a sine wave for a 3D effect
    float size_scale = 1.0f + 0.5f * sin(now); // Scale between 0.5 and 1.5
    entity->size = v2_mulf(entity->start_size, size_scale); // Use start_size for scaling

    // Draw the boss with the updated position and scaled size
    draw_centered_in_frame_rect(entity->position, entity->size, entity->color, frame);
    draw_centered_in_frame_rect(v2_add(entity->position, v2(entity->size.x, 0)), v2_mulf(entity->size, 0.3), entity->color, frame);
    draw_centered_in_frame_rect(v2_add(entity->position, v2(-entity->size.x, 0)), v2_mulf(entity->size, 0.3), entity->color, frame);
}

void draw_boss_health_bar(Draw_Frame* frame) {
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;
		
		if (entity->entitytype == ENTITY_BOSS) {
			float health_bar_height = 50;
			float health_bar_width = 150;
			float padding = 15;
			float margin = 5;
			float alpha = (float)entity->health / (float)entity->start_health;
			draw_rect_in_frame(v2(-health_bar_width / 2 - margin, window.height / 2 - health_bar_height - padding - margin), v2(health_bar_width + 2*margin, health_bar_height + 2*margin), v4(0.5, 0.5, 0.5, 0.5), frame);
			draw_rect_in_frame(v2(-health_bar_width / 2, window.height / 2 - health_bar_height - padding), v2(health_bar_width*alpha, health_bar_height), v4(0.9, 0.0, 0.0, 0.7), frame);
			string label = sprint(get_temporary_allocator(), STR("%i"), entity->health);
			u32 font_height = 48;
			Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));
			draw_text(font_bold, label, font_height, v2(-health_bar_width / 2 + health_bar_width*alpha / 2 - m.visual_size.x / 2, window.height / 2 - health_bar_height - padding + 10), v2(1, 1), COLOR_WHITE);
		}
	}
}

void draw_center_stage_text(Draw_Frame* frame)
{
	// Create the label using sprint
	string label = sprint(get_temporary_allocator(), STR("%i"), current_stage_level);
	u32 font_height = 48 + (current_stage_level*4);
	Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));
	draw_text_in_frame(font_bold, label, font_height, v2(-m.visual_size.x / 2, 0), v2(1, 1), COLOR_WHITE, frame);
}

void draw_effect_ui() {
	int y_diff = 0;
	for (int i=0; i < MAX_ENTITY_COUNT; i++) {
		Effect* effect = &world->effects[i];
	
		if (effect == NULL || !(effect->is_valid)) {
            continue;
        }

		if (effect->timer == NULL || !(effect->timer->is_valid)) {
            continue; // Skip this effect if the timer is not initialized
        }
			
		Vector2 effect_position = v2(window.width / 2 - 150, window.height / 2 - 30 - y_diff);

		Gfx_Text_Metrics m = measure_text(font_bold, effect_pretty_text(effect->effect_type), font_height, v2(0.4, 0.4));

		draw_centered_rect(v2_add(effect_position, v2(m.visual_size.x / 2, 0.75*m.visual_size.y / 2)), v2(1.25*m.visual_size.x, 1.25*m.visual_size.y), v4(0.5, 0.5, 0.5, 0.5));
		float a = 1.0f - effect->timer->progress;
		draw_rect(effect_position, v2(a*1.25*m.visual_size.x, 1.25*m.visual_size.y), COLOR_RED);
		draw_text(font_bold, sprint(get_temporary_allocator(), effect_pretty_text(effect->effect_type)), font_height, effect_position, v2(0.4, 0.4), COLOR_WHITE);
		
		y_diff += 25;
		if (timer_finished(effect->timer)) {
			destroy_timedevent(effect->timer);
			destroy_effect(effect);
		}
	}
}

void draw_starting_screen() {
	if (is_starting_screen_active) {
		is_game_paused = true;  // Ensure the game is paused when in the main screen

		// Draw a black background for the starting screen
		draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 1));

		// Create Play button
		string label = sprint(get_temporary_allocator(), STR("Not Pong"));
		u32 font_height = 48;
		float button_size = 200;
		Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));
		draw_text(font_bold, label, font_height, v2(-m.visual_size.x / 2, 75), v2(1, 1), COLOR_WHITE);

		// Play Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Play")), v2(-button_size / 2, 0), v2(button_size, 50), true)) {
			is_main_menu_active = false;  // Close menu
			is_game_paused = false;      
			is_starting_screen_active = false;
			is_game_running = true;
		}

		// Settings Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Settings")), v2(-button_size / 2, -50), v2(button_size, 50), true)) {
			is_main_menu_active = false;      // Close main menu
			is_settings_menu_active = true;   // Open settings menu
			is_starting_screen_active = false;
		}

		// Quit Option
		if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Quit")), v2(-button_size / 2, -100), v2(button_size, 50), true)) {
			window.should_close = true;  // Quit game
		}
	}
}

void draw_main_menu() {
	if (is_main_menu_active) {

		if (is_game_running) {
			draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 0.5));
		}
		else {
			draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 1));
		}

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

void draw_settings_menu() {
	if (is_settings_menu_active) {
		
		if (is_game_running) {
			draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 0.5));
		}
		else {
			draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 1));
		}

		// Create the label using sprint
		string label = sprint(get_temporary_allocator(), STR("Settings"));
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
			is_settings_menu_active = false;
			is_graphics_settings_active = true;
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

void draw_graphics_settings_menu() {
    if (is_graphics_settings_active) {

        if (is_game_running) {
			draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 0.5));
		}
		else {
			draw_rect(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), v4(0, 0, 0, 1));
		}
		
        // Create the label using sprint
        string label = sprint(get_temporary_allocator(), STR("Graphics Settings"));
        u32 font_height = 48;
        float button_width = 250;
        Gfx_Text_Metrics m = measure_text(font_bold, label, font_height, v2(1, 1));
        draw_text(font_bold, label, font_height, v2(-m.visual_size.x / 2, 75), v2(1, 1), COLOR_WHITE);

        Vector2 button_size = v2(button_width, 50);
        int y = 0;

        // Shader Toggle
        if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Shaders: %s"), use_shaders ? "ON" : "OFF"), 
                        v2(-button_size.x / 2, y), button_size, true)) {
            use_shaders = !use_shaders;
            // Here you would implement the logic to enable/disable shaders
        }
        y -= 60;

        // Dummy Resolution Setting
        if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Resolution: 1920x1080")), 
                        v2(-button_size.x / 2, y), button_size, true)) {
            // Dummy function - doesn't actually change resolution
        }
        y -= 60;

        // Dummy Fullscreen Toggle
        static bool dummy_fullscreen = false;
        if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Fullscreen: %s"), dummy_fullscreen ? "ON" : "OFF"), 
                        v2(-button_size.x / 2, y), button_size, true)) {
            dummy_fullscreen = !dummy_fullscreen;
            // Dummy function - doesn't actually toggle fullscreen
        }
        y -= 60;

        // Dummy VSync Toggle
        static bool dummy_vsync = true;
        if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("VSync: %s"), dummy_vsync ? "ON" : "OFF"), 
                        v2(-button_size.x / 2, y), button_size, true)) {
            dummy_vsync = !dummy_vsync;
            // Dummy function - doesn't actually toggle VSync
        }
        y -= 60;

        // Back Button to return to Settings Menu
        if (draw_button(font_light, font_height, sprint(get_temporary_allocator(), STR("Back")), 
                        v2(-button_size.x / 2, y), button_size, true)) {
            is_graphics_settings_active = false;  // Close graphics settings menu
            is_settings_menu_active = true;       // Return to settings menu
        }
    }
}

void draw_timed_events() {
    float y_offset = 0;  // Initialize y_offset to zero
    for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
        timed_event_info(&world->timedevents[i], i, &y_offset);
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

    int n_existing_particles = number_of_certain_particle(PFX_SNOW);
	int particles_to_spawn = calculate_particles_to_spawn((float)current_stage_level / 10.0f, n_existing_particles, 1000.0f);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), particles_to_spawn, PFX_SNOW);
}

void stage_10_boss() {
    Entity* boss = create_entity();
    setup_boss_stage_10(boss);

    int n_existing_particles = number_of_certain_particle(PFX_SNOW);
	int particles_to_spawn = calculate_particles_to_spawn((float)current_stage_level / 10.0f, n_existing_particles, 1000.0f);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), particles_to_spawn, PFX_SNOW);
}

void stage_11_to_19() {
	
	float stage_progress = current_stage_level - 10;
	float r = (float)stage_progress / 20.0f;
	float g = 0.0f;
	float b = 0.0f;
	world->world_background = v4(r, g, b, 1.0f); // Background color
	summon_world(SPAWN_RATE_ALL_OBSTACLES);

	int n_existing_particles = number_of_certain_particle(PFX_ASH);
	int particles_to_spawn = calculate_particles_to_spawn((float)stage_progress / 10.0f, n_existing_particles, 1000.0f);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), particles_to_spawn, PFX_ASH);
	if (number_of_certain_particle(PFX_SNOW)) remove_all_particle_type(PFX_SNOW);
}
void stage_20_boss() {
    Entity* boss = create_entity();
    setup_boss_stage_10(boss);

    int n_existing_particles = number_of_certain_particle(PFX_ASH);
	int particles_to_spawn = calculate_particles_to_spawn((float)current_stage_level / 10.0f, n_existing_particles, 1000.0f);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), particles_to_spawn, PFX_ASH);
}
void stage_21_to_29() {
	float r = 0.5f;
	float g = 0.2f;
	float b = 0.0f;  // Mörk orange/brun för lavabakgrund
	world->world_background = v4(r, g, b, 1.0f); // Background color for lava

	summon_world(SPAWN_RATE_ALL_OBSTACLES);

	// Emit aska (ash) partiklar
	int n_ash_particles = number_of_certain_particle(PFX_ASH);
	particle_emit(v2(0, 0), v4(0.5, 0.3, 0.1, 1.0), 500 * ((float)current_stage_level / (float)10) - n_ash_particles, PFX_ASH);
}

void stage_31_to_39() {
    float r = 0.5f;
    float g = 0.2f;
    float b = 0.0f;  // Mörk orange/brun för lavabakgrund
    world->world_background = v4(r, g, b, 1.0f); // Background color for lava

    summon_world(SPAWN_RATE_ALL_OBSTACLES);

    // Emit lövpartiklar
    int n_leaf_particles = number_of_certain_particle(PFX_LEAF);
    particle_emit(v2(0, 0), v4(0.1, 0.5, 0.1, 1.0), 500 * ((float)current_stage_level / (float)10) - n_leaf_particles, PFX_LEAF);
}

void stage_41_to_49() {
    float r = 0.5f;
    float g = 0.2f;
    float b = 0.0f;  // Mörk orange/brun för lavabakgrund
    world->world_background = v4(r, g, b, 1.0f); // Background color for lava

    summon_world(SPAWN_RATE_ALL_OBSTACLES);

    // Emit regnpartiklar
    int n_rain_particles = number_of_certain_particle(PFX_RAIN);
    particle_emit(v2(0, 0), v4(0.6, 0.6, 1.0, 0.5), 500 * ((float)current_stage_level / (float)10) - n_rain_particles, PFX_RAIN);
}

void stage_51_to_59() {
    float r = 0.5f;
    float g = 0.5f;
    float b = 0.8f;  // Bakgrundsfärg för vindnivå
    world->world_background = v4(r, g, b, 1.0f); // Sätt bakgrundsfärg

    summon_world(SPAWN_RATE_ALL_OBSTACLES);

    // Emit vindpartiklar
    int n_wind_particles = number_of_certain_particle(PFX_WIND);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), 1000 - n_wind_particles, PFX_WIND); // Anpassa antalet
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
	else if (current_stage_level == 20) 
	{
		stage_20_boss();
	}
	else 
	{
		summon_world(SPAWN_RATE_ALL_OBSTACLES);
	}
}

// -----------------------------------------------------------------------
//                   FUNCTION FOR DRAWING THE ENTIRE GAME
// -----------------------------------------------------------------------

void draw_game(Draw_Frame *frame) {
	draw_rect_in_frame(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), world->world_background, frame);
	draw_center_stage_text(frame);
	
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;

		if (entity->obstacle_type == OBSTACLE_BEAM)
		{
			draw_beam(entity, frame);
		}

		if (entity->entitytype == ENTITY_EFFECT)
		{
			float a = 0.2 * sin(7*now) + 0.8;
			Vector4 col = entity->color;
			entity->color = v4(col.x, col.y, col.z, a);

			draw_centered_in_frame_circle(entity->position, v2_add(entity->size, v2(2, 2)), COLOR_BLACK, frame);
			draw_centered_in_frame_circle(entity->position, entity->size, entity->color, frame);
		}

		if (entity->entitytype == ENTITY_PROJECTILE)
		{
			draw_centered_in_frame_circle(entity->position, v2_add(entity->size, v2(2, 2)), COLOR_BLACK, frame);
			draw_centered_in_frame_circle(entity->position, entity->size, entity->color, frame);
		}
	
		if (entity->entitytype == ENTITY_OBSTACLE)
		{
			switch(entity->obstacle_type) 
			{
				case(OBSTACLE_DROP): {
					entity->color = v4(1, 1, 1, 0.3);
				} break;
				case(OBSTACLE_BEAM): {
					entity->color = v4(0, 1, 0, 0.3);
				} break;
				case(OBSTACLE_HARD):
					float r = 0.5 * sin(now + 3*PI32) + 0.5;
					entity->color = v4(r, 0, 1, 1);
					break;
				case(OBSTACLE_BLOCK):
					float a = 0.3 * sin(2*now) + 0.7;
					entity->color = v4(0.2, 0.2, 0.2, a);
					break;
		
				default: { } break; 
			}

			draw_centered_in_frame_rect(entity->position, entity->size, entity->color, frame);
		}
	
		if (entity->entitytype == ENTITY_EFFECT) 
		{
			draw_centered_in_frame_circle(entity->position, entity->size, entity->color, frame);
		}

		if (entity->entitytype == ENTITY_PLAYER)
		{
			draw_centered_in_frame_rect(entity->position, entity->size, entity->color, frame);
		}

		if (entity->entitytype == ENTITY_BOSS) {
			draw_boss(entity, frame);
		}
	}

	// When a stage is cleared this is runned
	if (obstacle_count - number_of_block_obstacles <= 0 && !(boss_is_alive(world))) {
		current_stage_level++;
		initialize_new_stage(world, current_stage_level);
		window.clear_color = world->world_background;
	}

	if (!(is_game_paused)) { particle_update(); }
	
	int number_of_particles = particle_render(frame);
	
	// Check if game is over or not
	game_over = number_of_shots_missed >= number_of_hearts;

	if (game_over) {
		play_one_audio_clip(STR("res/sound_effects/Impact_038.wav"));
		current_stage_level = 0;
		number_of_shots_missed = 0;
		clean_world();
		remove_all_particle_type(PFX_SNOW);
		remove_all_particle_type(PFX_ASH);
		remove_all_particle_type(PFX_LEAF);
		remove_all_particle_type(PFX_RAIN);
		remove_all_particle_type(PFX_WIND);
		summon_world(SPAWN_RATE_ALL_OBSTACLES);
		draw_text(font_light, sprint(get_temporary_allocator(), STR("GAME OVER\nSURVIVED %i STAGES"), current_stage_level), font_height, v2(0, 0), v2(1, 1), COLOR_WHITE);
	}
	
	draw_hearts(frame);

	draw_boss_health_bar(frame);

	draw_playable_area_borders(frame);

	draw_death_borders(color_switch_event, frame);

	
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
	float64 last_time = os_get_elapsed_seconds();

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	Gfx_Shader_Extension light_shader;
	Gfx_Shader_Extension bloom_map_shader;
	Gfx_Shader_Extension postprocess_bloom_shader;

	// regular shader + point light which makes things extra bright
	light_shader = load_shader(STR("oogabooga/examples/bloom_light.hlsl"), sizeof(Scene_Cbuffer));
	// shader used to generate bloom map. Very simple: It takes the output color -1 on all channels 
	// so all we have left is how much bloom there should be
	bloom_map_shader = load_shader(STR("oogabooga/examples/bloom_map.hlsl"), sizeof(Scene_Cbuffer));
	// postprocess shader where the bloom happens. It samples from the generated bloom_map.
	postprocess_bloom_shader = load_shader(STR("oogabooga/examples/bloom.hlsl"), sizeof(Scene_Cbuffer));

	

	Scene_Cbuffer scene_cbuffer;

	
	Gfx_Image *bloom_map = 0;
	Gfx_Image *game_image = 0;
	Gfx_Image *final_image = 0;

	View_Mode view = VIEW_GAME_AFTER_POSTPROCESS;
	Draw_Frame offscreen_draw_frame;
	draw_frame_init(&offscreen_draw_frame);

	// Load in images and other stuff from disk
	font_light = load_font_from_disk(STR("./res/fonts/Abaddon Light.ttf"), get_heap_allocator());
	assert(font_light, "Failed loading './res/fonts/Abaddon Light.ttf'");

	font_bold = load_font_from_disk(STR("./res/fonts/Abaddon Bold.ttf"), get_heap_allocator());
	assert(font_bold, "Failed loading './res/fonts/Abaddon Bold.ttf'");
	
	heart_sprite = load_image_from_disk(STR("res/textures/heart.png"), get_heap_allocator());
	assert(heart_sprite, "Failed loading 'res/textures/heart.png'");

	effect_heart_sprite = load_image_from_disk(STR("res/textures/effect_heart.png"), get_heap_allocator());
	assert(effect_heart_sprite, "Failed loading 'res/textures/effect_heart.png'");

	// Here we create the player object
	
	player = create_player();
	
	summon_world(SPAWN_RATE_ALL_OBSTACLES);
	world->world_background = COLOR_BLACK;

	// -----------------------------------------------------------------------
	//                      WORLD TIMED TIMED_EVENTS ARE MADE HERE
	// -----------------------------------------------------------------------
	color_switch_event = initialize_color_switch_event(world);
	os_update();
	while (!window.should_close) {
		reset_temporary_storage();

		local_persist Os_Window last_window;
		if ((last_window.width != window.width || last_window.height != window.height || !game_image) && window.width > 0 && window.height > 0) {
			if (bloom_map)   delete_image(bloom_map);
			if (game_image)  delete_image(game_image);
			if (final_image) delete_image(final_image);
			
			bloom_map  = make_image_render_target(window.width, window.height, 4, 0, get_heap_allocator());
			game_image = make_image_render_target(window.width, window.height, 4, 0, get_heap_allocator());
			final_image = make_image_render_target(window.width, window.height, 4, 0, get_heap_allocator());
		}
		last_window = window;

		// Time stuff
		now = os_get_elapsed_seconds();
		
		delta_t = now - last_time;
		last_time = now;

		// Mouse Positions
		mouse_position = MOUSE_POSITION();

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
			if (is_starting_screen_active) {
				continue;
			} else if (is_graphics_settings_active) {
				is_graphics_settings_active = false;
				is_settings_menu_active = true;
			} else if (is_settings_menu_active) {
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

				Entity* projectile = create_entity();
				summon_projectile_player(projectile, player);

				number_of_shots_fired++;
			}

			else if (is_key_just_pressed(KEY_SPACEBAR)) 
			{
				consume_key_just_pressed(KEY_SPACEBAR);

				Entity* projectile = create_entity();
				summon_projectile_player(projectile, player);

				number_of_shots_fired++;
			}

			update_player_position(player);
		}

		// -----------------------------------------------------------------------
		//               Entity Loop for updating and collisions
		// -----------------------------------------------------------------------
		int entity_counter = 0;
		
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* entity = &world->entities[i];
			if (!entity->is_valid) continue;
			entity_counter++;

			// Update positon from velocity of entity if game is not paused
			if (!(is_game_paused)) entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));
			
			if (entity->entitytype == ENTITY_PROJECTILE) 
			{
				bool collision = false;
				for (int j = 0; j < MAX_ENTITY_COUNT; j++) 
				{
					Entity* other_entity = &world->entities[j];
					if (!other_entity->is_valid) continue;

					if (entity == other_entity) continue;
					if (collision) break;
					switch(other_entity->entitytype) {
						case(ENTITY_PLAYER):
						case(ENTITY_BOSS):
						case(ENTITY_OBSTACLE):{
							if (circle_rect_collision(entity, other_entity)) 
							{
								handle_projectile_collision(entity, other_entity);
								collision = true;
							} 
						} break;

						case(ENTITY_PROJECTILE): {
							if (circle_circle_collision(entity, other_entity))
							{
								particle_emit(other_entity->position, other_entity->color, 4, PFX_EFFECT);
								handle_projectile_collision(entity, other_entity);
								collision = true;
							}
						} break;

						case(ENTITY_EFFECT): {
							if (circle_circle_collision(entity, other_entity)) 
							{
								particle_emit(other_entity->position, other_entity->color, 4, PFX_EFFECT);
								handle_projectile_collision(entity, other_entity);

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

				if (entity->position.y <= -window.height / 2 || entity->position.y >= window.height / 2)
				{
					number_of_shots_missed++;
					play_one_audio_clip(STR("res/sound_effects/Impact_021.wav"));
					destroy_entity(entity);
				}
			}
			
			if (entity->obstacle_type == OBSTACLE_BEAM) {
				if (entity->child != NULL) {
					if (rect_rect_collision(entity->child, player->entity)) {
						if (!player->is_immune) {
							handle_beam_collision(entity->child, player);
							player->is_immune = true; // Sätt spelaren som immun
							player->immunity_timer = 1.0f; // Sätt timer för immunitet
						}
					}
				}
			}

			if (entity->entitytype == ENTITY_OBSTACLE) 
			{
				if (entity->obstacle_type == OBSTACLE_DROP) {
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
							Entity* drop_projectile = create_entity();
							summon_projectile_drop(drop_projectile, entity);
						}
					}
				}

				// Wave Effect
				if (entity->wave_time > 0.0f && entity->obstacle_type != OBSTACLE_BLOCK) {
					update_wave_effect(entity);
				}
			}

			if (entity->entitytype == ENTITY_PLAYER) {
				limit_player_position(player);
			}

			if (entity->entitytype == ENTITY_BOSS) {
				update_boss(entity);
			}
		}

		
		// Set stuff in cbuffer which we need to pass to shaders
		scene_cbuffer.mouse_pos_screen = v2(input_frame.mouse_x, input_frame.mouse_y);
		scene_cbuffer.window_size = v2(window.width, window.height);
		
		// Draw game with light shader to game_image
		scene_cbuffer.light_count = 0; // clean the list

		if (use_shaders) {
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* entity = &world->entities[i];
				if (scene_cbuffer.light_count >= MAX_LIGHTS) break;
				if (!entity->is_valid) continue;
				if (entity->entitytype == ENTITY_PROJECTILE || 
				    entity->entitytype == ENTITY_EFFECT ||
					entity->entitytype == ENTITY_OBSTACLE) {
					create_light_source(entity->position, entity->color, 100.0f, &scene_cbuffer);
				}
			}

			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Particle* p = &particles[i];
				if (scene_cbuffer.light_count >= MAX_LIGHTS) break;
				if (!(p->flags & PARTICLE_FLAGS_valid)) {
					continue;
				}
				if (p->kind == PFX_EFFECT || p->kind == PFX_BOUNCE) {
					create_light_source(p->pos, p->col, 10.0f, &scene_cbuffer);
				}
			}
		}
		
		// Reset draw frame & clear the image with a clear color
		draw_frame_reset(&offscreen_draw_frame);
		
		
		gfx_clear_render_target(game_image, v4(.7, .7, .7, 1.0));
		
		// Draw game things to offscreen Draw_Frame
		draw_game(&offscreen_draw_frame);
		

		// Set the shader & cbuffer before the render call
		offscreen_draw_frame.shader_extension = light_shader;
		offscreen_draw_frame.cbuffer = &scene_cbuffer;
		
		// Render Draw_Frame to the image
		///// NOTE: Drawing to one frame like this will wait for the gpu to finish the last draw call. If this becomes
		// a performance bottleneck, you would have more frames "in flight" which you cycle through.
		gfx_render_draw_frame(&offscreen_draw_frame, game_image);
		
		// Draw game with bloom map shader to the bloom map
		// Reset draw frame & clear the image
		draw_frame_reset(&offscreen_draw_frame);
		gfx_clear_render_target(bloom_map, COLOR_BLACK);
		
		// Draw game things to offscreen Draw_Frame
		
		draw_game(&offscreen_draw_frame);
		
		// Set the shader & cbuffer before the render call
		offscreen_draw_frame.shader_extension = bloom_map_shader;
		offscreen_draw_frame.cbuffer = &scene_cbuffer;
		
		gfx_render_draw_frame(&offscreen_draw_frame, bloom_map);
		
		// Draw game image into final image, using the bloom shader which samples from the bloom_map
		draw_frame_reset(&offscreen_draw_frame);
		
		gfx_clear_render_target(final_image, COLOR_BLACK);
		
		// To sample from another image in the shader, we must bind it to a specific slot.

		draw_frame_bind_image_to_shader(&offscreen_draw_frame, bloom_map, 0);
		
		// Draw the game the final image, but now with the post process shader
		draw_image_in_frame(game_image, v2(-window.width/2, -window.height/2), v2(window.width, window.height), COLOR_WHITE, &offscreen_draw_frame);
		
		
		offscreen_draw_frame.shader_extension = postprocess_bloom_shader;
		offscreen_draw_frame.cbuffer = &scene_cbuffer;
		gfx_render_draw_frame(&offscreen_draw_frame, final_image);
		
		draw_image(final_image, v2(-window.width/2, -window.height/2), v2(window.width, window.height), COLOR_WHITE);
		
		if (debug_mode) {
			draw_line(player->entity->position, mouse_position, 2.0f, v4(1, 1, 1, 0.5));
			draw_text(font_light, sprint(get_temporary_allocator(), STR("fps: %i (%.2f ms)"), latest_fps, (float)1000 / latest_fps), font_height, v2(-window.width / 2, window.height / 2 - 50), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("entities: %i"), latest_entites), font_height, v2(-window.width / 2, window.height / 2 - 75), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("obstacles: %i, block: %i"), obstacle_count, number_of_block_obstacles), font_height, v2(-window.width / 2, window.height / 2 - 100), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("destroyed: %i"), number_of_destroyed_obstacles), font_height, v2(-window.width / 2, window.height / 2 - 125), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("projectiles: %i"), number_of_shots_fired), font_height, v2(-window.width / 2, window.height / 2 - 150), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("particles: %i"), number_of_particles), font_height, v2(-window.width / 2, window.height / 2 - 175), v2(0.4, 0.4), COLOR_GREEN);
			draw_timed_events();
		}

		draw_effect_ui();

		draw_graphics_settings_menu();

		draw_settings_menu();

		draw_main_menu();

		draw_starting_screen();
	
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