#define m4_identity m4_make_scale(v3(1, 1, 1))
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

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