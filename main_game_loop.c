typedef struct Entity {
	Vector2 size;
	Vector2 position;
	Vector2 velocity;
	Vector4 color;
	bool is_alive;
} Entity;

void setup_player(Entity* entity) {
	entity->size = v2(100, 20);
	entity->position = v2(0, -300);
	entity->color = COLOR_WHITE;
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
	struct Entity player;
	setup_player(&player);
	// --------------------------------

	while (!window.should_close) {
		reset_temporary_storage();

		// Time stuff
		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		// main code loop here --------------
		// Here we draw the player as a rectangle
		{
			Vector2 input_axis = v2(0, 0); // Create an empty 2-dim vector
			if (is_key_down('A') && player.position.x > -window.point_width / 2 - player.size.x / 2) {
				input_axis.x -= 1.0;
			}
			if (is_key_down('D') && player.position.x <  window.point_width / 2 - player.size.x / 2) {
				input_axis.x += 1.0;
			}
			
			input_axis = v2_normalize(input_axis);
			player.position = v2_add(player.position, v2_mulf(input_axis, 500.0 * delta_t));
		}
		
		draw_rect(player.position, player.size, player.color);
		draw_circle(player.position, v2(5, 5), COLOR_RED); // Player position origin

		// main code loop here --------------
		os_update(); 
		gfx_update();
		seconds_counter += delta_t;
		frame_count += 1;
		if (seconds_counter > 1.0) {
			log("fps: %i", frame_count);
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}