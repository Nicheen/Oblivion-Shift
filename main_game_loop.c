
int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = STR("Minimal Game Example");
	window.point_width = 2560/2;
	window.point_height = 1440/2; 
	window.x = 2560/4;
	window.y = 1440/4;
	window.clear_color = hex_to_rgba(0x2a2d3aff);
	float64 seconds_counter = 0.0;
	s32 frame_count = 0;
	float64 last_time = os_get_elapsed_seconds();
	while (!window.should_close) {
		reset_temporary_storage();
		
		float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;
		Matrix4 rect_xform = m4_scalar(1.0);
		rect_xform         = m4_rotate_z(rect_xform, (f32)now);
		rect_xform         = m4_translate(rect_xform, v3(-125, -125, 0));
		draw_rect_xform(rect_xform, v2(250, 250), v4(0.2, 0, 0.3, 0.5));
		
		draw_rect(v2(sin(now)*window.width*0.4-60, -60), v2(120, 120), v4(1, 0, 0, 0.5));
		draw_rect(v2(-60, sin(now)*window.height*0.4-60), v2(120, 120), v4(0, 1, 0, 0.5));
		draw_rect(v2(-sin(now)*window.width*0.4-60, -60), v2(120, 120), v4(0, 1, 0, 0.5));
		draw_rect(v2(-60, -sin(now)*window.height*0.4-60), v2(120, 120), v4(1, 0, 0, 0.5));
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