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
    vertex_pos.y = window_size.y - vertex_pos.y; // Adjust for Y inversion

    float4 total_light_contribution = float4(0, 0, 0, 1);

    for (int i = 0; i < light_count; i++) {
        LightSource light = lights[i];

        // Point light calculation (skip if size is defined, i.e., it's a rectangular light)
        if (light.size.x == 0 && light.size.y == 0) {
            float dist = length(light.position - vertex_pos);
            if (dist > light.radius) continue; // Skip pixels outside of the light radius

            float attenuation = saturate(1.0 - (dist / light.radius)) * light.intensity;
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
                // Instead of using the distance, use the individual x and y components for attenuation
                float attenuation_x;

                // Uniform light falloff along x-axis if radius is 0
                if (light.radius == 0) {
                    attenuation_x = 1.0; // No falloff on x-axis
                } else {
                    // Taper off x-axis based on radius
                    attenuation_x = saturate(1.0 - (abs(rotated_pos.x) / light.radius));
                }
                float attenuation_y = saturate(1.0 - (abs(rotated_pos.y) / (light.size.y * 0.5)));
                float attenuation = attenuation_x * attenuation_y * light.intensity;

                total_light_contribution.rgb += light.color.rgb * max(attenuation, 0);
            }
        }
    }

    return total_light_contribution;
}


float4 pixel_shader_extension(PS_INPUT input, float4 color) {
    if (input.type == 1) {
        return color;
    }
    return color + get_light_contribution(input);
}