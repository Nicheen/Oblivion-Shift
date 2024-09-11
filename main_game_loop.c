// game variables
#define MAX_ENTITY_COUNT 1024
int number_of_destroyed_obstacles = 0;
int number_of_shots_fired = 0;
int number_of_shots_missed = 0;
int number_of_power_ups = 0;
bool debug_mode = false;
bool game_over = false;

inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

typedef struct Entity {
	// --- Entity Attributes ---
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	bool is_valid;
	// --- Entity Type Below ---
	// Player
	bool is_player;
	// Obstacle
	bool is_obstacle;
	int obstacle_health;
	// Projectile
	bool is_projectile;
	// Power UP
	bool is_power_up;
} Entity;

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

void setup_player(Entity* entity) {
	entity->size = v2(50, 20);
	entity->position = v2(0, -300);
	entity->color = COLOR_WHITE;

	entity->is_player = true;
}

void setup_projectile(Entity* entity, Entity* player) {
	entity->size = v2(10, 10);
	entity->position = player->position;
	entity->color = v4(0, 1, 0, 1); // Green color
	entity->velocity = v2(0, 200);

	entity->is_projectile = true;
}

void setup_obstacle(Entity* entity, int x_index, int y_index) {
	int size = 20;
	int padding = 10;
	entity->size = v2(size, size);
	entity->position = v2(-180, -45);
	entity->position = v2_add(entity->position, v2(x_index*(size + padding), y_index*(size + padding)));
	entity->color = v4(1, 0, 0, 1);
	
	if (get_random_float64_in_range(0, 1) <= 0.10) 
	{
		entity->obstacle_health = 2;
		entity->size = v2(30, 30);
	} 
	else 
	{
		entity->obstacle_health = 1;
	}

	entity->is_obstacle = true;
}

void setup_power_up(Entity* entity) {
	entity->size = v2(15, 15);
	entity->position = v2(0, 0);
	entity->color = v4(0, 0, 1, 1);
	entity->is_power_up = true;
	number_of_power_ups++;
}
bool is_out_of_bounds(Entity* entity) {
    return (entity->position.x < -window.width / 2 || 
            entity->position.x > window.width / 2 || 
            entity->position.y < -window.height / 2 || 
            entity->position.y > window.height / 2);
}

void apply_damage(Entity* obstacle, int damage) {
    obstacle->obstacle_health -= damage;
    if (obstacle->obstacle_health <= 0) {
		// Destroy the obstacle after its health is 0
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

void handle_projectile_collision(Entity* projectile, Entity* obstacle) {
    int damage = 1; // This can be changes in the future
    apply_damage(obstacle, damage);
    
    // Destroy the projectile after it hits the obstacle
    entity_destroy(projectile);
}

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
			Entity* entity = entity_create();
			setup_obstacle(entity, i, j);
			float red = 1 - (float)(i+1) / rows_obstacles;
			float blue = (float)(i+1) / rows_obstacles;
			entity->color = v4(red, 0, blue, 1);
		}
	}

	// --------------------------------
	while (!window.should_close) {
		reset_temporary_storage();

		// Time stuff
		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		// main code loop here --------------
		if (is_key_just_pressed(KEY_TAB)) 
		{
			consume_key_just_pressed(KEY_TAB);
			debug_mode = !debug_mode;  // Toggle debug_mode with a single line
		}

		if (is_key_just_pressed(KEY_ESCAPE) && game_over) {
			reset_values();
		}

		if (player->is_valid || game_over)
		{
			// Hantera vänsterklick
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT) || is_key_just_pressed(KEY_SPACEBAR)) 
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				Entity* projectile = entity_create();
				setup_projectile(projectile, player);

				number_of_shots_fired++;
			}

			if (number_of_destroyed_obstacles == 10 && number_of_power_ups == 0){
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
							handle_projectile_collision(entity, other_entity);
                    		break; // Exit after handling the first collision
						} 
					}
				}
			}
			
			// TODO fix this below
			if (is_out_of_bounds(entity) && entity->is_projectile)
			{
				number_of_shots_missed++;
				entity_destroy(entity);
			}

			if (entity->is_projectile) 
			{
				Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
				draw_circle(draw_position, entity->size, entity->color);
			}

			if (entity->is_player || entity->is_obstacle) 
			{
				Vector2 draw_position = v2_sub(entity->position, v2_mulf(entity->size, 0.5));
				draw_rect(draw_position, entity->size, entity->color);
				Vector2 draw_position_2 = v2_sub(entity->position, v2_mulf(v2(5, 5), 0.5));
				if (debug_mode) { draw_circle(draw_position_2, v2(5, 5), COLOR_GREEN); }
			}
		}

		draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_destroyed_obstacles), font_height, v2(-window.width / 2, 25 - window.height / 2), v2(0.7, 0.7), COLOR_RED);
		draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_shots_fired), font_height, v2(-window.width / 2, -window.height / 2), v2(0.7, 0.7), COLOR_GREEN);

		// Check if game is over or not
		game_over = number_of_shots_missed >= 3;

		if (game_over) {
			draw_text(font, sprint(get_temporary_allocator(), STR("GAME OVER"), number_of_shots_missed), font_height, v2(-window.width / 2, 0), v2(1.5, 1.5), COLOR_GREEN);
		}

		for (int i = 0; i < max(3 - number_of_shots_missed, 0); i++) {
			int heart_size = 30;
			int heart_padding = 10;
			Vector2 heart_position = v2(0, -window.height / 2);
			heart_position = v2_add(heart_position, v2((heart_size + heart_padding)*i, 0));
			draw_image(heart_sprite, heart_position, v2(heart_size, heart_size), v4(1, 1, 1, 1));
		}
		
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