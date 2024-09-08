#define MAX_ENTITY_COUNT 1024

typedef struct Entity {
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	bool is_circle;
	bool is_valid;
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
}

void setup_projectile(Entity* entity, Entity* player) {
	entity->size = v2(10, 10);
	entity->position = v2(player->position.x + player->size.x / 2, player->position.y + player->size.y);
	entity->color = v4(0, 1, 0, 1); // Green color
	entity->is_circle = true;
	entity->velocity = v2(0, get_random_int_in_range(100, 200));
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

	// Here we create the player entity object
	Entity* player = entity_create();
	setup_player(player);

	

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
				entity_counter += 1;
				entity->position = v2_add(entity->position, v2_mulf(entity->velocity, delta_t));

				if (entity->position.y > window.height) {
					entity_destroy(entity);
				}

				if (entity->is_circle) {
					draw_circle(entity->position, entity->size, entity->color);
				} else {
					draw_rect(entity->position, entity->size, entity->color);
				}
			}
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