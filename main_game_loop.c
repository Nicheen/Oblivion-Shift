#define MAX_ENTITY_COUNT 1024

inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

typedef struct Entity {
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	bool is_valid;
	// Entity Type Below
	bool is_obstacle;
	bool is_projectile;
	bool is_player;
	int obstacle_health;
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
	entity->size = v2(100, 20);
	entity->position = v2(0, -300);
	entity->color = COLOR_WHITE;

	entity->is_player = true;
}

void setup_projectile(Entity* entity, Entity* player) {
	entity->size = v2(10, 10);
	entity->position = player->position;
	entity->color = v4(0, 1, 0, 1); // Green color
	entity->velocity = v2(0, get_random_int_in_range(100, 200));

	entity->is_projectile = true;
}

void setup_obstacle(Entity* entity, int x_index, int y_index) {
	int size = 20;
	int padding = 10;
	entity->size = v2(size, size);
	entity->position = v2(x_index*(size + padding) - 190, y_index*(size + padding) - 100);
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

bool is_out_of_bounds(Entity* entity) {
    return (entity->position.x < -window.width / 2 || 
            entity->position.x > window.width / 2 || 
            entity->position.y < -window.height / 2 || 
            entity->position.y > window.height / 2);
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

	// game variables
	int number_of_destroyed_obstacles = 0;

	// Here we create the player entity object
	Entity* player = entity_create();
	setup_player(player);

	for (int i = 0; i < 13; i++) { // x
		for (int j = 0; j < 14; j++) { // y
			Entity* entity = entity_create();
			setup_obstacle(entity, i, j);
			float red = 1 - (float)(i+1) / 13;
			float blue = (float)(i+1) / 13;
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
		if (player->is_valid)
		{
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) 
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);
				Entity* projectile = entity_create();
				setup_projectile(projectile, player);
			}

			Vector2 input_axis = v2(0, 0); // Create an empty 2-dim vector
			
			if (is_key_down('A') && player->position.x > -window.point_width / 2 - player->size.x / 2)
			{
				input_axis.x -= 1.0;
			}

			if (is_key_down('D') && player->position.x <  window.point_width / 2 - player->size.x / 2)
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
			if (entity->is_valid) { // Check if the entity is valid or not
				entity_counter++;

				entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));
				if (entity->is_projectile) {
					for (int j = 0; j < MAX_ENTITY_COUNT; j++) {
						Entity* other_entity = &entities[j];
						if (other_entity->is_obstacle) {
							if (v2_dist(entity->position, other_entity->position) <= 0) {
								other_entity->obstacle_health -= 2;
								entity_destroy(entity);
							} else if (v2_dist(entity->position, other_entity->position) <= other_entity->size.x ) {
								other_entity->obstacle_health -= 1;
								entity_destroy(entity);
							}

							if (other_entity->obstacle_health <= 0) {
								entity_destroy(other_entity);
								number_of_destroyed_obstacles++;
							}
						}
					}
				}
				
				// TODO fix this below
				if (is_out_of_bounds(entity) && entity->is_projectile)
				{
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
					draw_circle(draw_position_2, v2(5, 5), COLOR_GREEN);
				}
			}
		}

		Vector2 text_position = player->position;
		draw_text(font, sprint(get_temporary_allocator(), STR("%i"), number_of_destroyed_obstacles), font_height, text_position, v2(0.7, 0.7), COLOR_RED);

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