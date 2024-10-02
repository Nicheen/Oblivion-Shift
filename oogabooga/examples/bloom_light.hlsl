// This is copypasted in bloom.hlsl and bloom_map.hlsl.
// You can do #include in hlsl shaders, but I wanted this example to be very simple to look at.

struct LightSource {
    float2 position;
    float intensity;
    float radius;
    float4 color;
    int type;
    float2 direction;
    float length;
};

#define MAX_LIGHTS 100

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

        if (light.type == 0)  // Point light
        {
            float dist = length(light.position - vertex_pos);
            float attenuation = saturate(1.0 - (dist / light.radius)) * light.intensity;

            // Avoid negative contributions
            total_light_contribution.rgb += light.color.rgb * max(attenuation, 0);
        } 
        else if (light.type == 1)  // Line light
        {
            if (length(light.direction) > 0.0) {  // Ensure valid direction vector
                float2 to_vertex = vertex_pos - light.position;
                float2 line_dir = normalize(light.direction);
                
                // Project vertex position onto the light's direction
                float proj_length = dot(to_vertex, line_dir);
                
                // Clamp the projection within the light's length
                proj_length = clamp(proj_length, 0.0, light.length);
                
                // Calculate the closest point on the line segment
                float2 closest_point = light.position + line_dir * proj_length;
                
                // Calculate the distance from the vertex to the closest point on the line
                float dist_to_line = length(closest_point - vertex_pos);
                
                // If the distance to the line is within the light's radius, calculate attenuation
                if (dist_to_line <= light.radius) {
                    float attenuation = saturate(1.0 - (dist_to_line / light.radius)) * light.intensity;
                    
                    // Fade out near the ends of the line segment
                    float end_fade = saturate(proj_length / light.radius) * 
                                     saturate((light.length - proj_length) / light.radius);
                    
                    attenuation *= end_fade;

                    // Avoid negative contributions
                    total_light_contribution.rgb += light.color.rgb * max(attenuation, 0);
                }
            }
        }
    }

    return total_light_contribution;
}

float4 pixel_shader_extension(PS_INPUT input, float4 color) {
    return color + get_light_contribution(input);
}