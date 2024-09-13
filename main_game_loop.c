// game variables
#define MAX_ENTITY_COUNT 1024

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
s32 delta_t;

float easeInQuart(float x) { // x inbetween 0-1
	return x * x * x * x;
}

inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

Vector2 screen_to_world() {
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

typedef enum ObstacleType {
	NIL_obstacle = 0,
	BASE_obstacle = 1,
	HARD_obstacle = 2,
	BLOCK_obstacle = 3,
	MAX_obstacle,
} ObstacleType;
	
typedef enum Power_Up_Type {
	NIL_power_up = 0,
	test_power_up_green = 1,
	test_power_up_red = 2,
	test_power_up_blue = 3,
	test_power_up_yellow = 4,
	Max_power_up,
} Power_Up_Type;

typedef struct Entity {
	// --- Entity Attributes ---
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	Vector4 original_color;
	bool is_valid;
	// --- Entity Type Below ---
	// Player
	bool is_player;
	// Obstacle
	bool is_obstacle;
	int obstacle_health;
	float wave_time;
	enum ObstacleType obstacle_type;
	// Projectile
	bool is_projectile;
	int n_bounces;
	// Power UP
	bool is_power_up;
	enum Power_Up_Type power_up_type;
} Entity;

// TODO: Probably Noel wants to add a struct for power ups to seperate from the entities.

// A list to hold all the entities
Entity entities[MAX_ENTITY_COUNT];

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

typedef struct ObstacleTuple {
	Entity* obstacle;
	int x;
	int y;
} ObstacleTuple;
ObstacleTuple obstacle_list[MAX_ENTITY_COUNT];
int obstacle_count = 0;

void setup_player(Entity* entity) {
	entity->size = v2(50, 20);
	entity->position = v2(0, -300);
	entity->color = COLOR_WHITE;

	entity->is_player = true;
}

void setup_projectile(Entity* entity, Entity* player) {
	int setup_y_offset = 30;
	entity->size = v2(10, 10);
	entity->position = v2_add(player->position, v2(0, setup_y_offset));
	entity->color = v4(0, 1, 0, 1); // Green color
	Vector2 normalized_velocity = v2_normalize(v2_sub(mouse_position, v2(player->position.x, player->position.y + setup_y_offset)));
	entity->velocity = v2_mulf(normalized_velocity, projectile_speed);

	entity->is_projectile = true;
}

void setup_power_up(Entity* entity) {
	int size = 25;
	entity->size = v2(size, size);
	entity->obstacle_health = 1;
	entity->is_power_up = true;
	number_of_power_ups++;

	float random_value = get_random_float64_in_range(0, 1);

	if (random_value < 0.25)
	{
		entity->power_up_type = test_power_up_blue;
		entity->color = COLOR_BLUE;
	} 
	else if (random_value < 0.50)
	{
		entity->power_up_type = test_power_up_green;
		entity->color = COLOR_GREEN;
	} 
	else if (random_value < 0.75)
	{
		entity->power_up_type = test_power_up_yellow;
		entity->color = COLOR_YELLOW;
	} 
	else
	{
		entity->power_up_type = test_power_up_red;
		entity->color = COLOR_RED;
	}
}

void setup_obstacle(Entity* entity, int x_index, int y_index, int n_rows) {
	int size = 20;
	int padding = 10;
	entity->size = v2(size, size);
	entity->position = v2(-180, -45);
	entity->position = v2_add(entity->position, v2(x_index*(size + padding), y_index*(size + padding)));
	
	// TODO: Make it more clear which block is harder
	float random_value = get_random_float64_in_range(0, 1);

	if (random_value <= 0.30) // 30% chance
	{
		// HARD obstacle
		entity->obstacle_type = HARD_obstacle;
		entity->obstacle_health = 2;
	} 
	else if (random_value <= 0.40)  // 10% chance
	{
		// BLOCK obstacle
		entity->obstacle_type = BLOCK_obstacle;
		entity->obstacle_health = 9999;
		entity->size = v2(30, 30);
	} 
	else
	{
		// BASE obstacle
		entity->obstacle_type = BASE_obstacle;
		entity->obstacle_health = 1;
		float red = 1 - (float)(x_index+1) / n_rows;
		float blue = (float)(x_index+1) / n_rows;
		entity->original_color = v4(red, 0, blue, 1);
	}
	
	obstacle_list[obstacle_count].obstacle = entity;
	obstacle_list[obstacle_count].x = x_index;
	obstacle_list[obstacle_count].y = y_index;
	obstacle_count++;
	entity->is_obstacle = true;
}


// TODO: implement projectile_bounce for obstacles with health > 1
// Problem arises with calculating damage when projectile survives
// The loop will run multiple of times before the projectile can bounce away.
void projectile_bounce(Entity* projectile, Entity* obstacle) {
	projectile->n_bounces++;
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
	projectile->velocity = v2_mul(projectile->velocity, v2(-1,  1)); // Bounce x-axis
}

void propagate_wave(Entity* hit_obstacle) {
    float wave_radius = 500.0f;  // Start with 0 radius and grow over time
	float wave_speed = 100.0f;
	float wave_duration = 0.1f;

    Vector2 hit_position = hit_obstacle->position;

    // Iterate over all obstacles to propagate the wave
    for (int i = 0; i < obstacle_count; i++) {
        Entity* current_obstacle = obstacle_list[i].obstacle;
		
        if (current_obstacle != hit_obstacle) {  // Skip the hit obstacle itself
            float distance = v2_dist(hit_position, current_obstacle->position);

            // If within the wave radius, apply wave effect
            if (distance <= wave_radius) {
				float wave_delay = distance / wave_speed;

                current_obstacle->wave_time = max(0.0f, wave_duration - wave_delay);  // Delay based on distance
            }
        }
    }
}

void apply_damage(Entity* obstacle, float damage) {
	obstacle->obstacle_health -= damage;

	if (obstacle->obstacle_health <= 0) {
		// Destroy the obstacle after its health is 0
		if (obstacle->is_obstacle) { propagate_wave(obstacle); }
		entity_destroy(obstacle);
		number_of_destroyed_obstacles += 1;
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

	if (obstacle->obstacle_type == BASE_obstacle || obstacle->obstacle_type == HARD_obstacle || obstacle->is_power_up) { 
		apply_damage(obstacle, damage);
	}
	
	if (obstacle->obstacle_type == BLOCK_obstacle) 
	{
		projectile_bounce(projectile, obstacle);
	} 
	else
	{
		entity_destroy(projectile);
	}
}

void apply_power_up(Entity* power_up, Entity* player) {
	if(power_up->power_up_type == test_power_up_green){
		death_zone_bottom = v4(0, 1, 0, 0.5);
		is_power_up_active = true;
		timer_power_up = 10.0f; //funkar ej ännu
	}
	if(power_up->power_up_type == test_power_up_yellow){
		death_zone_top = v4(0, 1, 0, 0.5);
	}
	if(power_up->power_up_type == test_power_up_blue){
		player->size = v2_add(player->size, v2(100, 0));
	}
	if(power_up->power_up_type == test_power_up_red){
		if (number_of_shots_missed > 0) {
			number_of_shots_missed--;
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

	int rows_obstacles = 13;
	int columns_obstacles = 13;
	for (int i = 0; i < rows_obstacles; i++) { // x
		for (int j = 0; j < columns_obstacles; j++) { // y
			if (get_random_float64_in_range(0, 1) <= 0.70) {
				Entity* entity = entity_create();
				setup_obstacle(entity, i, j, rows_obstacles);
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

		// Mouse Positions
		mouse_position = screen_to_world();

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
				setup_projectile(projectile, player);

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

			if (entity->is_projectile) 
			{
				for (int j = 0; j < MAX_ENTITY_COUNT; j++) 
				{
					Entity* other_entity = &entities[j];

					if (other_entity->is_obstacle)
					{
						if (circle_rect_collision(entity, other_entity)) 
						{
							//other_entity->wave_time = 0.0f;
							handle_projectile_collision(entity, other_entity);
							break; // Exit after handling the first collision
						}
					}
					if (other_entity->is_player) 
					{
						if (circle_rect_collision(entity, other_entity))
						{
							handle_projectile_collision(entity, other_entity);
							break; // Exit after handling the first collision
						} 
					}
					if (other_entity->is_power_up)
					{
						if (circle_circle_collision(entity, other_entity)) 
						{
							apply_power_up(other_entity, player);
							handle_projectile_collision(entity, other_entity);
							number_of_power_ups--;
                    		break; // Exit after handling the first collision
						}
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
				if (entity->is_projectile || entity->is_power_up) 
				{
					if(entity->is_power_up){
						entity->position = v2(window.width / 2 * sin(t + random_position_power_up), -100);
						float g = 0.2 * sin(10*t) + 0.8;
						float r = 0.2 * sin(10*t) + 0.8;
						float b = 0.2 * sin(10*t) + 0.8;
						switch(entity->power_up_type)
						{
							case(test_power_up_green):
								entity->color = v4(0, g, 0, 1);
								break;
							case(test_power_up_yellow):
								entity->color = v4(r, g, 0, 1);
								break;
							case(test_power_up_blue):
								entity->color = v4(0, 0, b, 1);
								break;
							case(test_power_up_red):
								entity->color = v4(r, 0, 0, 1);
								break;
							default: break;
						}
					}
					
					Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
					draw_circle(draw_position, entity->size, entity->color);
				}

				if (entity->is_player || entity->is_obstacle) 
				{
					if (entity->is_obstacle) 
					{
						switch(entity->obstacle_type) 
						{
							case(HARD_obstacle):
								float r = 0.5 * sin(t + 3*PI32) + 0.5;
								entity->original_color = v4(r, 0, 1, 1);
								break;
							case(BLOCK_obstacle):
								float a = 0.3 * sin(2*t) + 0.7;
								entity->original_color = v4(0.2, 0.2, 0.2, a);
								break;
							default: break;
						}
						if (entity->wave_time > 0.0f && entity->obstacle_type != BLOCK_obstacle) {
							entity->wave_time -= delta_t;

							if (entity->wave_time < 0.0f) {
								entity->wave_time = 0.0f;
							}

							// Animate obstacle as part of the wave (change color/size, etc.)
							float wave_intensity = entity->wave_time / 0.1f;  // Intensity decreases with time
							
							entity->color = v4(1.0f, 0.0f, 0.0f, wave_intensity);  // Example: red wave effect
						}
						else
						{
							entity->color = entity->original_color;
						}
					}
					Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
					draw_rect(draw_position, entity->size, entity->color);
					
					
					Vector2 draw_position_2 = v2_sub(entity->position, v2_mulf(v2(5, 5), 0.5));
					if (debug_mode) { draw_circle(draw_position_2, v2(5, 5), COLOR_GREEN); }
				}
				
			}
		}

		

		draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_destroyed_obstacles), font_height, v2(-window.width / 2, 25 - window.height / 2), v2(0.7, 0.7), COLOR_RED);
		draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_shots_fired), font_height, v2(-window.width / 2, -window.height / 2), v2(0.7, 0.7), COLOR_GREEN);

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
		draw_line(v2(-window.width / 2, -window.height / 2), v2(window.width / 2, -window.height/2), wave + 10.0f, death_zone_bottom);
		
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