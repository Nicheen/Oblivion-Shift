struct LightSource {
    float4 color;      // 16 bytes (4 floats)
    float2 position;   // 8 bytes (2 floats)
    float2 size;       // 8 bytes (2 floats)
    float2 direction;  // 8 bytes (2 floats)
    float intensity;   // 4 bytes
    float radius;      // 4 bytes
};

#define MAX_LIGHTS 40

cbuffer some_cbuffer : register(b0) {
    float2 mouse_pos_screen; // In pixels
    float2 window_size;
    LightSource lights[MAX_LIGHTS];
    int light_count;
}
float4 get_light_contribution(PS_INPUT input) {
	float2 vertex_pos = input.position_screen.xy;  // In pixels
    vertex_pos.y = window_size.y - vertex_pos.y; // For some reason D3D11 inverts the Y here, so we need to revert it

	float4 total_light_contribution = float4(0, 0, 0, 0);

	for (int i = 0; i < light_count; i++) {
		LightSource light = lights[i];

		// Point light calculation
        if (light.size.x == 0 && light.size.y == 0) {
            float dist = length(light.position - vertex_pos);
            if (dist > light.radius) continue; // Skip pixels outside of the light radius

            // Calculate attenuation based on distance and radius
            float attenuation = saturate(1.0 - (dist / light.radius)) * light.intensity;

            // Accumulate light contribution
            total_light_contribution.rgb += light.color.rgb * max(attenuation, 0);
        }
        // Rectangular light calculation
        else {
            // Rotate the vertex position to align with the light's local space
            float2 dir = normalize(light.direction);
            float2 rotated_pos = float2(
                dot(vertex_pos - light.position, dir),
                dot(vertex_pos - light.position, float2(-dir.y, dir.x))
            );

            // Check if within the rectangle bounds
            if (abs(rotated_pos.x) <= light.size.x * 0.5 && abs(rotated_pos.y) <= light.size.y * 0.5) {
                // Calculate attenuation based on the distance to the light center
                float dist = length(rotated_pos);
                float attenuation = saturate(1.0 - (dist / (max(light.size.x, light.size.y) * 0.5))) * light.intensity;

                // Accumulate light contribution
                total_light_contribution.rgb += light.color.rgb * max(attenuation, 0);
            }
        }
	}

	return total_light_contribution;
}

float4 pixel_shader_extension(PS_INPUT input, float4 color) {
	// We want to output everything above 1.0
    if (input.type == 1) {
        return color;
    }
	color = color + get_light_contribution(input);
	
	return float4(
		max(color.r-1.0, 0),
		max(color.g-1.0, 0),
		max(color.b-1.0, 0),
		color.a
	);
}