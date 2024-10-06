#define m4_identity m4_make_scale(v3(1, 1, 1))
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

Vector2 normalize(Vector2 v) {
    float length = sqrt(v.x * v.x + v.y * v.y);
    if (length > 0) {
        return (Vector2){ v.x / length, v.y / length };
    }
    return (Vector2){ 0.0f, 0.0f };  // Return a zero vector if length is zero
}

Gfx_Shader_Extension load_shader(string file_path, int cbuffer_size) {
    string source;
    
    // Attempt to read the shader file
    bool ok = os_read_entire_file(file_path, &source, get_heap_allocator());
    assert(ok, "Could not read shader file: %s", file_path);
    
    Gfx_Shader_Extension shader;

    // Compile the shader
    ok = gfx_compile_shader_extension(source, cbuffer_size, &shader);
    
    // Enhanced error message with shader file context
    assert(ok, "Failed compiling shader: %s", file_path);
    
    return shader;
}


Draw_Quad ndc_quad_to_screen_quad(Draw_Quad ndc_quad) {

	// NOTE: we're assuming these are the screen space matricies.
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;

	Matrix4 ndc_to_screen_space = m4_identity;
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, m4_inverse(proj));
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, view);

	ndc_quad.bottom_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_left), 0, 1)).xy;
	ndc_quad.bottom_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_right), 0, 1)).xy;
	ndc_quad.top_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_left), 0, 1)).xy;
	ndc_quad.top_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_right), 0, 1)).xy;

	return ndc_quad;
}

Vector2 world_pos_to_ndc(Vector2 world_pos) {

	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;

	Matrix4 world_space_to_ndc = m4_identity;
	world_space_to_ndc = m4_mul(proj, m4_inverse(view));

	Vector2 ndc = m4_transform(world_space_to_ndc, v4(v2_expand(world_pos), 0, 1)).xy;
	return ndc;
}

Vector2 ndc_pos_to_screen_pos(Vector2 ndc) {
	float w = window.width;
	float h = window.height;
	Vector2 screen = ndc;
	screen.x = (ndc.x * 0.5f + 0.5f) * w;
	screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * h;
	return screen;
}

inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

float float_alpha(float x, float min, float max) {
	float res = (x-min) / (max-min);
	res = clamp(res, 0.0, 1.0);
	return res;
}

float alpha_from_end_time(float64 current_time, float64 end_time, float length) {
	return float_alpha(current_time, end_time-length, end_time);
}

bool has_reached_end_time(float64 current_time, float64 end_time) {
	return current_time > end_time;
}

bool is_position_outside_bounds(Vector2 pos, Vector2 window_size) {
    // Check if the position is within the window boundaries
    return (pos.x > window_size.x / 2 || pos.x < -window_size.x / 2 ||
            pos.y > window_size.y / 2 || pos.y < -window_size.y / 2);
}

bool is_position_outside_walls_and_bottom(Vector2 pos, Vector2 window_size) {
    return (pos.x > window_size.x / 2 || pos.x < -window_size.x / 2 ||
            pos.y < -window_size.y / 2);
}

bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target_with_epsilon(float* value, float target, float delta_t, float rate, float epsilon) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, epsilon))
	{
		*value = target;
		return true; // reached
	}
	return false;
}
bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	return animate_f32_to_target_with_epsilon(value, target, delta_t, rate, 0.001f);
}

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

Vector4 rgba(float r, float g, float b, float a) {
	float alpha_r = (float)r / (float)255;
	float alpha_g = (float)g / (float)255;
	float alpha_b = (float)b / (float)255;
	float alpha_a = (float)a / (float)255;
	return v4(alpha_r, alpha_g, alpha_b, alpha_a);
}