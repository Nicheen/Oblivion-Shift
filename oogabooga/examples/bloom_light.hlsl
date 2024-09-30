struct PointLight {
	float2 position;
	float radius;
	float intensity;
	float4 color;
};

static const int LIGHT_MAX = 256;

cbuffer some_cbuffer : register(b0) {
    float2 mouse_pos_screen; // In pixels
    float2 window_size;
    PointLight point_lights[LIGHT_MAX];
	int point_light_count;
}

float4 pixel_shader_extension(PS_INPUT input, float4 color) {
	// Combined lighting pass
	float total_illumination = 0.0;
	float3 accumulated_light_color = float3(0.0, 0.0, 0.0);
	float2 pixel_pos = input.position_screen.xy; // Screen space coordinates

	// Loop over point lights, including the one at the mouse position
	for (int i = 0; i < point_light_count; ++i) {
		PointLight light = point_lights[i];

		// Compute vector from light to pixel in screen space
		// Adjust light position based on the projection transformation
		float2 light_dir = pixel_pos - light.position; // Calculate direction from light to pixel
		float distance = length(light_dir); // Calculate distance to light

		// Check if within light radius
		if (distance < light.radius) {
			// Calculate attenuation (using smoothstep for smoother falloff)
			float attenuation = smoothstep(light.radius, 0.0, distance) * light.intensity;

			// Accumulate total illumination
			total_illumination += attenuation;

			// Accumulate the light color contribution
			accumulated_light_color += light.color.rgb * attenuation;
		}
	}

	// Clamp total_illumination to [0, 1]
	total_illumination = saturate(total_illumination);

	// Blend the accumulated lighting with the original color
	color.rgb = lerp(color.rgb, accumulated_light_color, total_illumination);

	// Clamp the final color
	color.rgb = saturate(color.rgb);

	return color;
}

