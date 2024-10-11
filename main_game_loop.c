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
int entity_counter = 0;
const u32 font_height = 48;
LightSource lights[MAX_LIGHTS];
float stage_times[100];
float stage_timer;

float charge_time_projectile;
bool enhanced_projectile_damage = true;
bool enhanced_projectile_speed = false;
Vector4 barrier_color;

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
bool is_game_paused = true;
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
Gfx_Image* bloom_map = 0;
Gfx_Image* game_image = 0;
Gfx_Image* final_image = 0;

// -----------------------------------------------------------------------
//                  CREATE FUNCTIONS FOR ARRAY LOOKUP
// -----------------------------------------------------------------------
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
	if (entity->timer != NULL)        destroy_timedevent(entity->timer);
	if (entity->second_timer != NULL) destroy_timedevent(entity->second_timer);
	if (entity->third_timer != NULL)  destroy_timedevent(entity->third_timer);
	if (entity->child != NULL)        destroy_entity(entity->child);
	memset(entity, 0, sizeof(Entity));
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
	// Progress calculation
	timed_event->progress = timed_event->interval_timer / timed_event->interval;

	// Ensure progress is clamped between 0 and 1
	timed_event->progress = fminf(fmaxf(timed_event->progress, 0.0f), 1.0f);

	// Check if the timer has exceeded the switch interval
	if (timed_event->interval_timer >= timed_event->interval) {

		if (timed_event->duration_timer < timed_event->duration)
		{
			if (!is_game_paused && is_game_running) timed_event->duration_timer += delta_t;
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
	else
	{
		// Update the timer with the elapsed time
    	if (!is_game_paused && is_game_running) timed_event->interval_timer += delta_t;
	}

	return false;
}

TimedEvent* initialize_color_switch_event() {
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

TimedEvent* initialize_drop_event() {
	TimedEvent* te = create_timedevent(world);

	te->type = TIMED_EVENT_DROP;
	te->worldtype = TIMED_EVENT_TYPE_ENTITY;
	te->interval = 10.0f;
	te->interval_timer = get_random_float32_in_range(0, 0.7*te->interval);
	te->duration = 10.0f;
	te->progress = 0.0f;
	te->counter = 0;

	return te;
}

TimedEvent* initialize_beam_event() {
	TimedEvent* te = create_timedevent(world);

	te->type = TIMED_EVENT_BEAM;
	te->worldtype = TIMED_EVENT_TYPE_ENTITY;
	te->interval = 10.0f;
	te->interval_timer = get_random_float32_in_range(0, 0.7*te->interval);
	te->duration = 2.0f;
	te->progress = 0.0f;
	te->counter = 0;

	return te;
}

TimedEvent* initialize_boss_movement_event() {
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

TimedEvent* initialize_boss_movement_event_stage_30() {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_BOSS_MOVEMENT;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = 3.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

TimedEvent* initialize_boss_attack_event_stage_10() {
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

TimedEvent* initialize_boss_attack_event_20() {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_BOSS_ATTACK;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = 5.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

TimedEvent* initialize_boss_attack_event_stage_30() {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_BOSS_ATTACK;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = 0.5f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = 0;

    return te;
}

TimedEvent* initialize_boss_second_stage_event_stage_30() {
	TimedEvent* te = create_timedevent(world);

    te->type = TIMED_EVENT_EFFECT;
    te->worldtype = TIMED_EVENT_TYPE_ENTITY;
    te->interval = 30.0f; // Interval for new random values
    te->interval_timer = 0.0f;
    te->duration = 0.0f;
    te->duration_timer = 0.0f;
    te->progress = 0.0f;
    te->counter = -1;

    return te;
}

TimedEvent* initialize_effect_event(float duration) {
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

void create_rectangular_light_source(Vector2 position, Vector4 color, Vector2 size, float radius, Vector2 direction, float intensity, Scene_Cbuffer* scene_cbuffer) {
    LightSource light;

	light.position = v2_add(position, v2(window.width / 2, window.height / 2)); // Adjust position
    light.size = size;
	light.direction = normalize(direction); // Ensure direction is normalized
	light.radius = radius;

    light.color = color;
	light.intensity = intensity;
    scene_cbuffer->lights[scene_cbuffer->light_count] = light;
    scene_cbuffer->light_count++;
}

void create_circular_light_source(Vector2 position, Vector4 color, float intensity, float radius, Scene_Cbuffer* scene_cbuffer) {
    LightSource light;

	light.position = v2_add(position, v2(window.width / 2, window.height / 2)); // Adjust position
    light.size = v2(0, 0); // Set size to zero for point lights
	light.radius = radius;   // Use size for point light radius
	light.direction = v2(0, 0); // No direction needed for point lights

    light.color = color;
	light.intensity = intensity;
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
					if (p->kind == PFX_WIND) 
					{
						if (p->pos.x < -window.width / 2) 
						{
							p->pos = v2(window.width / 2, get_random_float32_in_range(-window.height / 2, window.height / 2));
							p->velocity = v2(get_random_float32_in_range(-30, -10), get_random_float32_in_range(-5, 5));
						}
					}
					else 
					{
						p->velocity = v2(get_random_float32_in_range(-10, 10), p->identifier * -20);
						p->pos = v2(get_random_float32_in_range(-window.width / 2, window.width / 2), get_random_float32_in_range(window.height / 2, 3 * window.height));
					}
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
				p->col = v4(0.7, 0.7, 1.0, 0.01); // Ljusblå färg för vinda
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
	entity->damage = 1;
	Vector2 normalized_velocity = v2_normalize(v2_sub(mouse_position, v2(player->entity->position.x, player->entity->position.y + setup_y_offset)));
	entity->velocity = v2_mulf(normalized_velocity, projectile_speed);
}

void summon_projectile_drop(Entity* entity, Entity* obstacle) {
	entity->entitytype = ENTITY_PROJECTILE;
	
	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = v2_sub(obstacle->position, v2(0, obstacle->size.y + entity->size.y));
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

void summon_beam(Entity* entity, Vector2 spawn_pos) {
    entity->obstacle_type = OBSTACLE_BEAM;
    entity->drop_interval = get_random_float32_in_range(3.0f, 5.0f);  // Time between beams
    entity->drop_duration_time = 1.0f;  // Beam lasts for 1 second
    entity->drop_interval_timer = 0.0f;
}

void summon_projectile_drop_boss_stage_30(Entity* entity, Vector2 spawn_pos) {
	entity->entitytype = ENTITY_PROJECTILE;

	entity->size = v2(10, 10);
	entity->health = 1;
	entity->max_bounces = 100;
	entity->position = spawn_pos;
	entity->color = v4(1, 0, 0, 0.5);
	entity->velocity = v2(0, -75);
}

void setup_beam(Entity* beam_obstacle, Entity* beam) {
	TimedEvent* timed_event = initialize_beam_event();
	beam->timer = timed_event;
	beam->color = v4(1, 0, 0, 0.7);

	float beam_height = beam_obstacle->size.y + window.height;
	beam->size = v2(5, beam_height);
	float beam_y_position = beam_obstacle->position.y - beam_height / 2 - beam_obstacle->size.y / 2;
	beam->position = v2(beam_obstacle->position.x, beam_y_position);
	beam->is_visible = false; // Initially invisible, until needed
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

	effect->timer = initialize_effect_event(effect->effect_duration);
}

void setup_boss_stage_10(Entity* entity) {
	entity->entitytype = ENTITY_BOSS;

	entity->health = 10;
	entity->start_health = entity->health;
	entity->color = rgba(165, 242, 243, 255);
	entity->size = v2(50, 50);
	entity->start_size = entity->size;

	entity->timer = initialize_boss_movement_event();
	entity->second_timer = initialize_boss_attack_event_stage_10();
}

void setup_boss_stage_20(Entity* entity) {
	entity->entitytype = ENTITY_BOSS;

	entity->health = 10;
	entity->start_health = entity->health;
	entity->color = rgba(240, 100, 100, 255);
	entity->size = v2(80, 25);
	entity->start_size = entity->size;

	entity->timer = initialize_boss_movement_event(); // Boss movement event

	// Beam Functionality of the boss
	Entity* beam = create_entity();
	entity->child = beam;
	entity->child->timer = initialize_beam_event(); // Boss attack event
}

void setup_boss_stage_30(Entity* entity) {
    entity->entitytype = ENTITY_BOSS;

    entity->first_stage_boss_stage_30 = true;
    entity->health = 10;
    entity->start_health = entity->health;
    entity->color = rgba(20, 50, 243, 255);
    entity->size = v2(50, 50);
    entity->start_size = entity->size;
    entity->position = v2(entity->position.x , 200);
    entity->timer = initialize_boss_movement_event_stage_30();
    entity->second_timer = initialize_boss_attack_event_stage_30();
    entity->third_timer = initialize_boss_second_stage_event_stage_30();
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
		entity->color = v4(0.5, 0.7, 0.2, 1.0);
		entity->timer = initialize_drop_event();
		entity->child = create_entity();
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
        entity->drop_interval = get_random_float32_in_range(10.0f, 20.0f);  // Tid mellan strålar
        entity->drop_duration_time = 1.0f;  // Strålen varar i 1 sekund
        entity->drop_interval_timer = 0.0f;
		entity->color = COLOR_GREEN;

		Entity* beam = create_entity();
		setup_beam(entity, beam);
		entity->child = beam;
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

string get_timed_event_type_text(int timer_type) {
	switch (timer_type) {
		case TIMED_EVENT_COLOR_SWITCH: return STR("Color Switch");
		case TIMED_EVENT_BEAM: return STR("Laser Beam");
		case TIMED_EVENT_BOSS_MOVEMENT: return STR("Boss Movement");
		case TIMED_EVENT_BOSS_ATTACK: return STR("Boss Attack");
		case TIMED_EVENT_EFFECT: return STR("Effect");
		case TIMED_EVENT_DROP:return STR("Drop");
		default: return STR("NOT DEFINED");
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

	stage_timer = 0;
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
        v2(-window.width / 2, window.height / 2 - 225 - *y_offset),  // Adjust y-position based on y_offset
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

bool rect_rect_collision(Entity* rect1, Entity* rect2, bool rect1_centered, bool rect2_centered) {
    float rect1_left, rect1_right, rect1_top, rect1_bottom;
    float rect2_left, rect2_right, rect2_top, rect2_bottom;

    // Calculate the boundaries for rect1 based on whether it's centered
    if (rect1_centered) {
        rect1_left = rect1->position.x - rect1->size.x / 2.0f;
        rect1_right = rect1->position.x + rect1->size.x / 2.0f;
        rect1_top = rect1->position.y + rect1->size.y / 2.0f;
        rect1_bottom = rect1->position.y - rect1->size.y / 2.0f;
    } else {
        rect1_left = rect1->position.x;
        rect1_right = rect1->position.x + rect1->size.x;
        rect1_top = rect1->position.y + rect1->size.y;
        rect1_bottom = rect1->position.y;
    }

    // Calculate the boundaries for rect2 based on whether it's centered
    if (rect2_centered) {
        rect2_left = rect2->position.x - rect2->size.x / 2.0f;
        rect2_right = rect2->position.x + rect2->size.x / 2.0f;
        rect2_top = rect2->position.y + rect2->size.y / 2.0f;
        rect2_bottom = rect2->position.y - rect2->size.y / 2.0f;
    } else {
        rect2_left = rect2->position.x;
        rect2_right = rect2->position.x + rect2->size.x;
        rect2_top = rect2->position.y + rect2->size.y;
        rect2_bottom = rect2->position.y;
    }

    // Check if there is no overlap in any direction
    if (rect1_left > rect2_right || rect1_right < rect2_left ||
        rect1_bottom > rect2_top || rect1_top < rect2_bottom) {
        return false; // No collision
    }

    return true; // Collision detected
}

// -----------------------------------------------------------------------
//                   HANDLE COLLISION FUNCTIONS
// -----------------------------------------------------------------------

void handle_projectile_collision(Entity* projectile, Entity* obstacle) {

    int damage = projectile->damage; 
	
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

void handle_beam_collision(Entity* entity) {
    if (entity->child != NULL) {
		if (entity->child->is_visible) {
			if (rect_rect_collision(entity->child, player->entity, false, true)) {
				if (!player->is_immune) {
					number_of_shots_missed++;
					
					player->is_immune = true; 		// Sätt immunitet och starta timer
					player->immunity_timer = 1.0f;  // 1 sekunds immunitet
				}
			}
		}
    }
}

// -----------------------------------------------------------------------
//                         PLAYER FUNCTIONS
// -----------------------------------------------------------------------
void update_player(Player* player) {
    // Hantera immunitetstiden
    if (player->is_immune) {
        player->immunity_timer -= delta_t;  // Minska timer för varje frame
        
        if (player->immunity_timer <= 0.0f) {
            // När timern har gått ut, återställ immuniteten
            player->is_immune = false;
            player->immunity_timer = 0.0f;  // Sätt timer till 0 (även om det är redundant, det klargör att immuniteten är slut)
        }
    }
}

void update_player_position(Player* player) {
    Vector2 input_axis = v2(0, 0);  // Initialize input axis
    bool moving = false;
    static int previous_input = -1;  // Direction tracking
    static bool can_dash = true;     // Track if the player can dash
    static float dash_cooldown = 0.5f; // Cooldown time in seconds
    static float dash_timer = 0.5f;   // Timer to track cooldown
    static bool shift_released = true; // Track if shift has been released
    float dash_distance = 50.0f;    // Distance to teleport when dashing

    // Update dash cooldown timer
    if (!can_dash) {
        dash_timer += delta_t;
        if (dash_timer >= dash_cooldown) {
            can_dash = true;  // Reset dash availability
            dash_timer = 0.0f;
        }
    }

    // Check for left movement (A key or left arrow)
    if ((is_key_down('A') || is_key_down(KEY_ARROW_LEFT)) && player->entity->position.x > -PLAYABLE_WIDTH / 2 + player->entity->size.x / 2) {
        input_axis.x -= 1.0f;
        moving = true;

        // Decelerate if switching direction
        if (previous_input == 1 && player->entity->velocity.x > 0) {
            player->entity->velocity.x -= player->entity->deceleration.x * delta_t;
            if (player->entity->velocity.x < 0) player->entity->velocity.x = 0;
            return;
        }
        previous_input = 0;  // Update direction to left
    }

    // Check for right movement (D key or right arrow)
    if ((is_key_down('D') || is_key_down(KEY_ARROW_RIGHT)) && player->entity->position.x < PLAYABLE_WIDTH / 2 - player->entity->size.x / 2) {
        input_axis.x += 1.0f;
        moving = true;

        // Decelerate if switching direction
        if (previous_input == 0 && player->entity->velocity.x < 0) {
            player->entity->velocity.x += player->entity->deceleration.x * delta_t;
            if (player->entity->velocity.x > 0) player->entity->velocity.x = 0;
            return;
        }
        previous_input = 1;  // Update direction to right
    }

    // If both keys are pressed, stop moving
    if ((is_key_down('A') || is_key_down(KEY_ARROW_LEFT)) && (is_key_down('D') || is_key_down(KEY_ARROW_RIGHT))) {
        input_axis.x = 0;
        moving = false;
        previous_input = -1;  // No active movement
    }

    // Check if shift has been released
    if (!is_key_down('S')) {
        shift_released = true;  // Reset shift_released when shift is not pressed
    }

    // Dash (teleport) when shift is pressed AND shift was previously released
    if (is_key_down('S') && shift_released && can_dash && (is_key_down('A') || is_key_down('D'))) {
        if (previous_input == 0) {
            // Dashing to the left
            player->entity->position.x -= dash_distance;
            if (player->entity->position.x < -PLAYABLE_WIDTH / 2 + player->entity->size.x / 2) {
                player->entity->position.x = -PLAYABLE_WIDTH / 2 + player->entity->size.x / 2;  // Prevent out of bounds
            }
        } else if (previous_input == 1) {
            // Dashing to the right
            player->entity->position.x += dash_distance;
            if (player->entity->position.x > PLAYABLE_WIDTH / 2 - player->entity->size.x / 2) {
                player->entity->position.x = PLAYABLE_WIDTH / 2 - player->entity->size.x / 2;  // Prevent out of bounds
            }
        }
        can_dash = false;  // Start cooldown
        shift_released = false;  // Prevent further dashes until shift is released
    }

    // Normalize input_axis for proper direction
    input_axis = v2_normalize(input_axis);

    // Calculate speed modifier based on mouse button state
    float speed_multiplier = (is_key_down(MOUSE_BUTTON_RIGHT)) ? 0.5f : 1.0f;  // Move at half speed if right mouse is held down

    // Acceleration
    if (moving) {
        // Accelerate based on input
        player->entity->velocity = v2_add(player->entity->velocity, v2_mulf(input_axis, player->entity->acceleration.x * delta_t * speed_multiplier));

        // Cap velocity at max speed
        if (v2_length(player->entity->velocity) > player->max_speed * speed_multiplier) {
            player->entity->velocity = v2_mulf(v2_normalize(player->entity->velocity), player->max_speed * speed_multiplier);
        }
    } else {
        // Deceleration if no keys are pressed
        if (v2_length(player->entity->velocity) > player->min_speed) {
            Vector2 decel_vector = v2_mulf(v2_normalize(player->entity->velocity), -player->entity->deceleration.x * delta_t);
            player->entity->velocity = v2_add(player->entity->velocity, decel_vector);

            // Stop if near zero speed
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

void handle_player_attack(Entity* projectile) {
	// Adjust damage based on charge time
	int charge_based_damage = 1;  // Default damage is 1
	if (enhanced_projectile_damage) {
		// If damage enhancement is active
		if (charge_time_projectile >= 3.0f) {
			charge_based_damage = 3;  // 3 damage if charged for 3 or more seconds
		} else if (charge_time_projectile >= 1.0f) {
			charge_based_damage = 2;  // 2 damage if charged for 1 or more seconds
		}
	} else {
		// If speed enhancement is active, set damage to default (1)
		charge_based_damage = 1;  // Always 1 damage if speed enhancement is active
	}
	
	projectile->size = v2_mulf(v2(charge_based_damage, charge_based_damage), 5);
	projectile->damage = charge_based_damage;  // Set damage based on the active enhancement

	// Set projectile speed
	projectile_speed = 500.0f; // Default speed
	if (enhanced_projectile_speed) {
		if (charge_time_projectile >= 2.0f) {
			projectile_speed *= 2.0f;
		}
	}
	// Assuming your projectile has a velocity field, set the speed
	projectile->velocity = v2_mulf(v2_normalize(projectile->velocity), projectile_speed); // Apply speed
	
	charge_time_projectile = 0;  // Reset charge time after firing
	
	number_of_shots_fired++;
}

// -----------------------------------------------------------------------
//                   UPDATE FUNCTIONS FOR UPDATE LOOP
// -----------------------------------------------------------------------

void update_boss_position_if_over_limit(Entity* boss) {
    // Uppdatera bossens position baserat på dess hastighet
    boss->position.x += boss->velocity.x * delta_t;  // Uppdatera x-position

    // Kontrollera gränserna och byta riktning
    if (boss->position.x < -PLAYABLE_WIDTH / 2 + boss->size.x / 2) {
        // Om bossen når vänster gräns, byt riktning
        boss->velocity.x = fabs(boss->velocity.x);  // Sätt hastigheten positiv (höger)
        boss->position.x = -PLAYABLE_WIDTH / 2 + boss->size.x / 2;  // Se till att bossen stannar inom gränsen
    } else if (boss->position.x > PLAYABLE_WIDTH / 2 - boss->size.x / 2) {
        // Om bossen når höger gräns, byt riktning
        boss->velocity.x = -fabs(boss->velocity.x);  // Sätt hastigheten negativ (vänster)
        boss->position.x = PLAYABLE_WIDTH / 2 - boss->size.x / 2;  // Se till att bossen stannar inom gränsen
    }
}

Vector2 update_boss_stage_10_velocity(Vector2 velocity) 
{
    // Example of modifying the velocity based on a sine wave
    float velocity_amplitude = get_random_float32_in_range(10.0, 15.0); // Amplitude for velocity changes
    float new_velocity_x = velocity_amplitude * (sin(now) + 0.5f * sin(2.0f * now)); // Modify x velocity
    float new_velocity_y = velocity_amplitude * (sin(now + 0.5f) + 0.3f * sin(3.0f * now)); // Modify y velocity

    return v2(new_velocity_x, new_velocity_y); // Return updated velocity
}

void update_boss_stage_10(Entity* entity) 
{
	// Update the velocity based on the timer
    if (timer_finished(entity->timer)) {
        entity->velocity = update_boss_stage_10_velocity(entity->velocity);
    }

	if (timer_finished(entity->second_timer)) {
		Entity* p1 = create_entity();
		Entity* p2 = create_entity();
		summon_icicle(p1, v2_add(entity->position, v2(entity->size.x, 0)));
		summon_icicle(p2, v2_sub(entity->position, v2(entity->size.x, 0)));
	}

	update_boss_position_if_over_limit(entity);
}

Vector2 update_boss_stage_20_position(Vector2 current_position) 
{
    // Definiera teleportationsområden eller slumpa fram en ny position
    float new_x = get_random_float32_in_range(-PLAYABLE_WIDTH/2 + 40.0f, PLAYABLE_WIDTH/2 - 40.0f);  // Slumpa x-position inom skärmens bredd
    float new_y = get_random_float32_in_range(-(window.height/2) + 200, (window.height/2) - 100);     // Slumpa y-position inom övre halvan av skärmen

    Vector2 new_position = v2(new_x, new_y);  // Skapa en ny position med slumpmässiga värden

    return new_position;  // Returnera den nya positionen
}

Vector2 update_boss_stage_20_velocity(Vector2 velocity) 
{
    // Example of modifying the velocity based on a sine wave
    float velocity_amplitude = get_random_float32_in_range(10.0, 15.0); // Amplitude for velocity changes
    float new_velocity_x = velocity_amplitude * (sin(now) + 0.5f * sin(2.0f * now)); // Modify x velocity
    float new_velocity_y = velocity_amplitude * (sin(now + 0.5f) + 0.3f * sin(3.0f * now)); // Modify y velocity

    return v2(new_velocity_x, new_velocity_y); // Return updated velocity
}

void update_boss_stage_20(Entity* entity) 
{	// Update the velocity based on the timer
    if (timer_finished(entity->timer)) {
        entity->velocity = update_boss_stage_20_velocity(entity->velocity);
    }  

    static float teleport_timer = 0.0f; // Timer to track teleportation delay (in seconds)

    if (entity->child != NULL) {
        if (timer_finished(entity->child->timer)) {
            summon_beam(entity->child, v2_sub(entity->position, v2(entity->size.x, 0))); // Create the beam at the correct position
			handle_beam_collision(entity);
            teleport_timer = 2.0f; // Start the teleport delay (1 second)
        }

        // If beam has fired, start the teleportation countdown
        if (entity->child->is_visible) {
            teleport_timer -= delta_t; // Decrease the timer by the frame's time

            // Once the timer reaches 0 or less, teleport the boss
            if (teleport_timer <= 0.0f) {
                entity->position = update_boss_stage_20_position(entity->position); // Teleport the boss
            }
        }
    }

	update_boss_position_if_over_limit(entity);
}

void update_boss_stage_30(Entity* entity) {
    // Om andra timern (för attacker) har löpt ut, skapa en projektil
    if (timer_finished(entity->second_timer)) {
        Entity* p1 = create_entity();
        summon_projectile_drop_boss_stage_30(p1, v2_add(entity->position, v2(0, -entity->size.y)));

        // Om bossen är i andra fasen, förläng intervallet för nästa attack
        if (!entity->first_stage_boss_stage_30) {
            entity->second_timer->interval = 3.0f;  // Exempel: Gör attacker långsammare i andra fasen
        }
    }

    // Om tredje timern (övergång till andra fasen) har löpt ut
    if (timer_finished(entity->third_timer)) {
        entity->first_stage_boss_stage_30 = false;  // Övergång till andra fasen
        entity->velocity = v2(0, 0);  // Nollställ hastigheten

        // När bossen går in i andra fasen, justera attackintervallet
        entity->second_timer->interval = 5.0f;  // Öka intervallet (sakta ner attacker) i andra fasen
    }
	update_boss_position_if_over_limit(entity);
}

Vector2 update_boss_stage_30_position(Vector2 current_position) {
	float new_x = get_random_float32_in_range(-PLAYABLE_WIDTH/2 + 40.0f, PLAYABLE_WIDTH/2 - 40.0f);
	Vector2 new_position = v2(new_x, 200); 

    return new_position;
}

Vector2 update_boss_stage_30_velocity(Vector2 velocity, Entity* entity) {
	float velocity_amplitude = 100.0f;
    if (entity->first_stage_boss_stage_30) {
        // Sinusrörelse i första fasen
        float new_velocity_x = velocity_amplitude * (sin(now) + 0.5f * sin(2.0f * now)); 
        return v2(new_velocity_x, 0);  // Returnera x-hastighet för sinusrörelse
    } else {
        // I andra fasen ska hastigheten vara noll
        return v2(0, 0);  // Ingen rörelse, noll hastighet
    }
}

void update_wave_effect(Entity* entity) 
{
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

void update_obstacle_drop(Entity* entity) {
	int x = entity->grid_position.x;
	int y = entity->grid_position.y;
	if (check_clearance_below(world->obstacle_list, obstacle_count, x, y)) {
		if (timer_finished(entity->timer)) {
			entity->child->is_visible = true;
			if (entity->timer->duration_timer <= 0.5f) 
			{
				if (entity->child == NULL) entity->child = create_entity();
				summon_projectile_drop(entity->child, entity);
			}
			if (circle_rect_collision(entity->child, player->entity)) {
				handle_projectile_collision(entity->child, player->entity);
			}
		}
		else
		{
			entity->child->position = entity->position;
			entity->child->size = v2_mulf(v2(1,1), 10 * entity->timer->progress);
			entity->child->color = v4_lerp(COLOR_GREEN, COLOR_RED, entity->timer->progress);
			entity->child->is_visible = false;
		}
	}
}

// -----------------------------------------------------------------------
//                   DRAW FUNCTIONS FOR DRAW LOOP
// -----------------------------------------------------------------------

void draw_drop(Entity* entity, Draw_Frame* frame) {
	draw_centered_in_frame_circle(entity->child->position, entity->child->size, entity->child->color, frame);					
}

void draw_beam(Entity* entity, Draw_Frame* frame) {
    if (entity->child != NULL) {
        float max_beam_size = 5.0f;

		entity->child->is_visible = false;

        if (timer_finished(entity->child->timer)) {
			entity->child->is_visible = true;

            // Draw the real beam (red), centered on the entity
			float beam_shoot_alpha = powf(min(entity->child->timer->duration_timer, 1.0f), 3);

			entity->child->size = v2_mul(v2(max_beam_size, entity->position.y + window.height / 2), v2(1, beam_shoot_alpha));
			entity->child->position = v2_sub(entity->position, v2(entity->child->size.x / 2, entity->child->size.y + entity->size.y / 2));
			
            draw_rect_in_frame(entity->child->position, entity->child->size, COLOR_WHITE, frame);
        }
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
	
	barrier_color = color;
	draw_line_in_frame(v2(-window.width / 2,  window.height / 2), v2(window.width / 2,  window.height/2), wave, barrier_color, frame);
	draw_line_in_frame(v2(-window.width / 2, -window.height / 2), v2(window.width / 2, -window.height/2), wave, barrier_color, frame);

}

void draw_hearts() {
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

void draw_boss_stage_10(Entity* entity, Draw_Frame* frame) {
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

void draw_boss_stage_20(Entity* entity, Draw_Frame* frame) {  

	draw_beam(entity, frame);

    // Draw the boss with the updated position and scaled size
	draw_centered_in_frame_circle(entity->position, entity->start_size, entity->color, frame);
    
    // Justera y-värdet för att rita cirkeln högre upp
    Vector2 new_position = entity->position;
    new_position.y += 10;  

    draw_centered_in_frame_circle(new_position, v2(23, 48), entity->color, frame);

	Vector2 position_kube = entity->position;
    position_kube.y -= 7;  
	draw_centered_in_frame_rect(position_kube, v2(18, 18), entity->color, frame);
}	

void draw_boss_stage_30(Entity* entity, Draw_Frame* frame) {
    if (timer_finished(entity->timer)) {
        if (entity->first_stage_boss_stage_30) {
            // Endast uppdatera velocity och position i första fasen
            entity->velocity = update_boss_stage_30_velocity(entity->velocity, entity);
            entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));
        } else {
            // I andra fasen ska bossen bara teleportera sig
            entity->position = update_boss_stage_30_position(entity->position);
        }
    }
    draw_centered_in_frame_rect(entity->position, entity->size, entity->color, frame);
}

void draw_boss_health_bar() {
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;
		
		if (entity->entitytype == ENTITY_BOSS) {
			float health_bar_height = 50;
			float health_bar_width = 150;
			float padding = 15;
			float margin = 5;
			float alpha = (float)entity->health / (float)entity->start_health;
			draw_rect(v2(-health_bar_width / 2 - margin, window.height / 2 - health_bar_height - padding - margin), v2(health_bar_width + 2*margin, health_bar_height + 2*margin), v4(0.5, 0.5, 0.5, 0.5));
			draw_rect(v2(-health_bar_width / 2, window.height / 2 - health_bar_height - padding), v2(health_bar_width*alpha, health_bar_height), v4(0.9, 0.0, 0.0, 0.7));
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

void draw_stage_timers() {
    u32 font_height = 48;
    float base_y = 100; // Base Y position for the green text
	if (debug_mode) { base_y -= 200; }; // Shift the draw element if debug mode is enabled
	float base_x = -350; // Base X position for the green text
    float spacing = 25; // Spacing between texts

	Vector4 current_timer_color = COLOR_GREEN;
	Vector4 other_times_color = COLOR_WHITE;

    // Draw the green text (current stage timer)
    string label = sprint(get_temporary_allocator(), STR("%.2f"), stage_timer);
    draw_text(font_bold, label, font_height, v2(base_x, base_y), v2(1, 1), current_timer_color);

    // Limit the number of white texts to be drawn to a max of 5
    int max_white_times = 5;
    int times_to_draw = min(current_stage_level, max_white_times);

    // Draw white times below the green text, newest at the top with fading effect
    for (int i = 0; i < times_to_draw; i++) {
        // Calculate transparency based on position: newest is opaque, older ones fade
        float alpha = 1.0f - (i / (float)max_white_times);

        // Set fading white color with variable alpha
        Vector4 faded_white = v4(other_times_color.r, other_times_color.g, other_times_color.b, alpha);

        // Newest time should appear right below the green text
        string label = sprint(get_temporary_allocator(), STR("%.2f"), stage_times[current_stage_level - i]);
        draw_text(font_bold, label, font_height, v2(base_x, base_y - (i+1) * spacing), v2(0.4, 0.4), faded_white);
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
	float stage_progress = current_stage_level - 10;
    Entity* boss = create_entity();
    setup_boss_stage_20(boss);

    int n_existing_particles = number_of_certain_particle(PFX_ASH);
	int particles_to_spawn = calculate_particles_to_spawn((float)stage_progress / 10.0f, n_existing_particles, 1000.0f);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), particles_to_spawn, PFX_ASH);
}

void stage_21_to_29() {
	float r = 0.1f;
	float g = 0.2f;
	float b = 0.5f;  // Mörk orange/brun för lavabakgrund
	world->world_background = v4(r, g, b, 1.0f); // Background color for lava

	summon_world(SPAWN_RATE_ALL_OBSTACLES);

	int n_wind_particles = number_of_certain_particle(PFX_WIND);
	particle_emit(v2(0, 0), v4(0, 0, 0, 0), 100 * ((float)current_stage_level / (float)10) - n_wind_particles, PFX_WIND);
}

void stage_30_boss() {
    Entity* boss = create_entity();
    setup_boss_stage_30(boss);

    int n_existing_particles = number_of_certain_particle(PFX_WIND);
	int particles_to_spawn = calculate_particles_to_spawn((float)current_stage_level / 10.0f, n_existing_particles, 100.0f);
    particle_emit(v2(0, 0), v4(0, 0, 0, 0), particles_to_spawn, PFX_WIND);
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
	
	stage_times[current_stage_level] = stage_timer;
	
	clean_world();

	if (current_stage_level <= 9) 
	{
		stage_0_to_9();
	} 
	else if (current_stage_level == 10) 
	{
		stage_10_boss();
	} 
	else if (current_stage_level <= 19) 
	{
		stage_11_to_19();
	} 
	else if (current_stage_level == 20) 
	{
		stage_20_boss();
	}
	else if (current_stage_level <= 29) 
	{
		stage_21_to_29();
	} 
	else if (current_stage_level == 30) 
	{
		stage_30_boss();
	}
	else if (current_stage_level <= 39) 
	{
		stage_31_to_39();
	} 
	else 
	{
		summon_world(SPAWN_RATE_ALL_OBSTACLES);
	}
}

// -----------------------------------------------------------------------
//
//
//                           THE BIG FUNCTIONS
//
//
// -----------------------------------------------------------------------

void draw_game(Draw_Frame *frame) {
	draw_rect_in_frame(v2(-window.width / 2, -window.height / 2), v2(window.width, window.height), world->world_background, frame);
	draw_center_stage_text(frame);
	
	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;

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
			draw_centered_in_frame_rect(entity->position, entity->size, entity->color, frame);

			switch(entity->obstacle_type) 
			{
				case(OBSTACLE_DROP):  draw_drop(entity, frame); break;
				case(OBSTACLE_BEAM):  draw_beam(entity, frame); break;
				case(OBSTACLE_HARD):  entity->color = v4(0.5 * sin(now + 3*PI32) + 0.5, 0, 1, 1); break;
				case(OBSTACLE_BLOCK): entity->color = v4(0.2, 0.2, 0.2, 0.3 * sin(2*now) + 0.7); break;
		
				default: { } break; 
			}
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
			switch(current_stage_level) {
				case(10): draw_boss_stage_10(entity, frame); break;
				case(20): draw_boss_stage_20(entity, frame); break;
				case(30): draw_boss_stage_30(entity, frame); break;
				default: break;
			} break;
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

	draw_playable_area_borders(frame);

	draw_death_borders(color_switch_event, frame);
}

void update_game() {
	entity_counter = 0;

	for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
		Entity* entity = &world->entities[i];
		if (!entity->is_valid) continue;
		entity_counter++;

		// Update position from velocity if the game is not paused
		if (!(is_game_paused)) entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));

		switch (entity->entitytype) {
			case ENTITY_PROJECTILE: {
				bool collision = false;
				for (int j = 0; j < MAX_ENTITY_COUNT; j++) {
					Entity* other_entity = &world->entities[j];
					if (!other_entity->is_valid) continue;
					if (entity == other_entity) continue;
					if (collision) break;

					switch (other_entity->entitytype) {
						case ENTITY_PLAYER:
						case ENTITY_BOSS:
						case ENTITY_OBSTACLE: {
							if (circle_rect_collision(entity, other_entity)) {
								handle_projectile_collision(entity, other_entity);
								collision = true;
							}
						} break;

						case ENTITY_PROJECTILE: {
							if (circle_circle_collision(entity, other_entity)) {
								particle_emit(other_entity->position, other_entity->color, 4, PFX_EFFECT);
								handle_projectile_collision(entity, other_entity);
								collision = true;
							}
						} break;

						case ENTITY_EFFECT: {
							if (circle_circle_collision(entity, other_entity)) {
								particle_emit(other_entity->position, other_entity->color, 4, PFX_EFFECT);
								handle_projectile_collision(entity, other_entity);
								collision = true;
							}
						} break;

						default: break;
					}
				}

				// Check projectile bounds
				if (entity->position.x <= -PLAYABLE_WIDTH / 2 || entity->position.x >= PLAYABLE_WIDTH / 2) {
					projectile_bounce_world(entity);
					play_one_audio_clip(STR("res/sound_effects/vägg_thud.wav"));
				}

				if (entity->position.y <= -window.height / 2 || entity->position.y >= window.height / 2) {
					number_of_shots_missed++;
					play_one_audio_clip(STR("res/sound_effects/Impact_021.wav"));
					destroy_entity(entity);
				}
			} break;

			case ENTITY_OBSTACLE: {
				switch (entity->obstacle_type) {
					case OBSTACLE_DROP: update_obstacle_drop(entity); break;
					case OBSTACLE_BEAM: handle_beam_collision(entity); break;
					default: break;
				}

				// Handle wave effects for obstacles (if applicable)
				if (entity->wave_time > 0.0f && entity->obstacle_type != OBSTACLE_BLOCK) {
					update_wave_effect(entity);
				}
			} break;

			case ENTITY_PLAYER: {
				limit_player_position(player);
			} break;

			case ENTITY_BOSS: {
				switch(current_stage_level) {
					case(10): update_boss_stage_10(entity); break;
					case(20): update_boss_stage_20(entity); break;
					case(30): update_boss_stage_30(entity); break;
					default: break;
				}
			} break;

			default: break;
		}
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
	float64 last_time = os_get_elapsed_seconds();

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	Gfx_Shader_Extension light_shader;
	Gfx_Shader_Extension bloom_map_shader;
	Gfx_Shader_Extension postprocess_bloom_shader;

	// regular shader + point light which makes things extra bright
	light_shader = load_shader(STR("include/bloom_light.hlsl"), sizeof(Scene_Cbuffer));
	// shader used to generate bloom map. Very simple: It takes the output color -1 on all channels 
	// so all we have left is how much bloom there should be
	bloom_map_shader = load_shader(STR("include/bloom_map.hlsl"), sizeof(Scene_Cbuffer));
	// postprocess shader where the bloom happens. It samples from the generated bloom_map.
	postprocess_bloom_shader = load_shader(STR("include/bloom.hlsl"), sizeof(Scene_Cbuffer));

	Scene_Cbuffer scene_cbuffer;
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
	color_switch_event = initialize_color_switch_event();
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
		if (is_game_running && !is_game_paused) stage_timer += delta_t;

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
	
		if (player->entity->is_valid && !game_over && !is_game_paused) {
			// Track charging time while the right mouse button is held down
			if (is_key_down(MOUSE_BUTTON_RIGHT)) {
				charge_time_projectile += delta_t;  // Increase charge time
			}

			// Toggle between enhanced damage and enhanced speed with 'E'
			if (is_key_just_pressed('E')) {
				// Toggle the abilities
				if (enhanced_projectile_damage) {
					enhanced_projectile_damage = false;
					enhanced_projectile_speed = true;  // Enable speed enhancement
				} else {
					enhanced_projectile_damage = true;
					enhanced_projectile_speed = false;  // Enable damage enhancement
				}
				// Reset the charge time when toggling enhancements
				charge_time_projectile = 0.0f;  // Reset the charge time to prevent unintended effects
			}
		


			if (player->entity->is_valid && !game_over && !is_game_paused) {
				
				static bool has_played_sound_1 = false;
				static bool has_played_sound_2 = false;

				if (!has_played_sound_1 && enhanced_projectile_damage && charge_time_projectile >= 1.0f) {
						play_one_audio_clip(STR("res/sound_effects/new-notification-7-210334edited.wav"));
						has_played_sound_1 = true; // Sätt flaggan till true så att ljudet inte spelas igen
					}

				if (!has_played_sound_2 && enhanced_projectile_damage && charge_time_projectile >= 3.0f) {
						play_one_audio_clip(STR("res/sound_effects/system-notification-199277edited.wav"));
						has_played_sound_2 = true; // Sätt flaggan till true så att ljudet inte spelas igen
					}

				if (!has_played_sound_2 && enhanced_projectile_speed && charge_time_projectile >= 2.0f) {
						play_one_audio_clip(STR("res/sound_effects/system-notification-199277edited.wav"));
						has_played_sound_2 = true; // Sätt flaggan till true så att ljudet inte spelas igen
					}
				
				if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
					consume_key_just_pressed(MOUSE_BUTTON_LEFT);

					// Create a new projectile and set its properties
					Entity* projectile = create_entity();
					summon_projectile_player(projectile, player);

					handle_player_attack(projectile);
				}

				if (charge_time_projectile == 0) {
					has_played_sound_1 = false; // Återställ flaggan när projektilen laddas om
					has_played_sound_2 = false;
				}
				// Update player position as usual
				update_player_position(player);
				update_player(player);
			}
		}

		update_game();

		// Set stuff in cbuffer which we need to pass to shaders
		scene_cbuffer.mouse_pos_screen = v2(input_frame.mouse_x, input_frame.mouse_y);
		scene_cbuffer.window_size = v2(window.width, window.height);
		
		// Draw game with light shader to game_image
		scene_cbuffer.light_count = 0; // clean the list

		if (use_shaders) {
			create_rectangular_light_source(v2(0, -window.height / 2), barrier_color, v2(window.width, 40 + 10*(sin(now) + 3)), 0, v2(1, 0), 0.5f, &scene_cbuffer);
			create_rectangular_light_source(v2(0,  window.height / 2), barrier_color, v2(window.width, 40 + 10*(sin(now) + 3)), 0, v2(1, 0), 0.5f, &scene_cbuffer);
			
			create_rectangular_light_source(v2(-PLAYABLE_WIDTH / 2, 0), COLOR_WHITE, v2(window.height, 10), 0, v2(0, 1), 0.5f, &scene_cbuffer);
			create_rectangular_light_source(v2( PLAYABLE_WIDTH / 2, 0), COLOR_WHITE, v2(window.height, 10), 0, v2(0, 1), 0.5f, &scene_cbuffer);

			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* entity = &world->entities[i];
				if (scene_cbuffer.light_count >= MAX_LIGHTS) break;
				if (!entity->is_valid) continue;

				// Use rectangular lights for ENTITY_OBSTACLE
				if (entity->entitytype == ENTITY_OBSTACLE) {
					create_circular_light_source(entity->position, entity->color, 0.3f, entity->size.x * 2, &scene_cbuffer);

					if (entity->obstacle_type == OBSTACLE_BEAM) {
						if (entity->child->is_visible) {
							create_rectangular_light_source(v2_add(entity->child->position, v2_mulf(entity->child->size, 0.5)), COLOR_RED, v2(entity->child->size.y, entity->child->size.x * 5.0f), 0, v2(0, 1), 1.0f, &scene_cbuffer);
						}
					}
				}
				// Use circular lights for projectiles and effects
				else if (entity->entitytype == ENTITY_PROJECTILE || entity->entitytype == ENTITY_EFFECT) {
					create_circular_light_source(entity->position, entity->color, 0.3f, entity->size.x * 1.5f, &scene_cbuffer);
				}
				else if (entity->entitytype == ENTITY_BOSS) {
					if (entity->child != NULL) {
						if (entity->child->is_visible) {
							create_rectangular_light_source(v2_add(entity->child->position, v2_mulf(entity->child->size, 0.5)), COLOR_RED, v2(entity->child->size.y, entity->child->size.x * 5.0f), 0, v2(0, 1), 1.0f, &scene_cbuffer);
						}
					}
				}
			}

			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Particle* p = &particles[i];
				if (scene_cbuffer.light_count >= MAX_LIGHTS) break;
				if (!(p->flags & PARTICLE_FLAGS_valid)) continue;

				// Use point lights for certain particle effects
				if (p->kind == PFX_EFFECT || p->kind == PFX_BOUNCE) {
					create_circular_light_source(p->pos, p->col, 0.3f, 10.0f, &scene_cbuffer);
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
			draw_text(font_light, sprint(get_temporary_allocator(), STR("Stage: %i"), current_stage_level), font_height, v2(-window.width / 2, window.height / 2 - 25), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("fps: %i (%.2f ms)"), latest_fps, (float)1000 / latest_fps), font_height, v2(-window.width / 2, window.height / 2 - 50), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("entities: %i"), entity_counter), font_height, v2(-window.width / 2, window.height / 2 - 75), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("obstacles: %i, block: %i"), obstacle_count, number_of_block_obstacles), font_height, v2(-window.width / 2, window.height / 2 - 100), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("destroyed: %i"), number_of_destroyed_obstacles), font_height, v2(-window.width / 2, window.height / 2 - 125), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("projectiles: %i"), number_of_shots_fired), font_height, v2(-window.width / 2, window.height / 2 - 150), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("particles: %i"), number_of_particles), font_height, v2(-window.width / 2, window.height / 2 - 175), v2(0.4, 0.4), COLOR_GREEN);
			draw_text(font_light, sprint(get_temporary_allocator(), STR("light sources: %i"), scene_cbuffer.light_count), font_height, v2(-window.width / 2, window.height / 2 - 200), v2(0.4, 0.4), COLOR_GREEN);
			draw_timed_events();
		}

		draw_hearts();

		draw_boss_health_bar();

		draw_stage_timers();

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
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}