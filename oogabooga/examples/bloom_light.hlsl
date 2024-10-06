// This is copypasted in bloom.hlsl and bloom_map.hlsl.
// You can do #include in hlsl shaders, but I wanted this example to be very simple to look at.

struct LightSource {
    float2 position;
    float intensity;
    float radius; // Not used for rectangular light, kept for point lights
    float4 color;
    float2 size; // X and Y dimensions for the rectangle
    float2 direction; // Normalized vector for light rotation
};

#define MAX_LIGHTS 30

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

        float dist = length(light.position - vertex_pos);

        if (dist > light.radius) continue; // Skip pixels outside of the light radius

        float attenuation = saturate(1.0 - (dist / light.radius)) * light.intensity;

        // Avoid negative contributions
        total_light_contribution.rgb += light.color.rgb * max(attenuation, 0);
    }

    return total_light_contribution;
}

float4 pixel_shader_extension(PS_INPUT input, float4 color) {
    return color + get_light_contribution(input);
}