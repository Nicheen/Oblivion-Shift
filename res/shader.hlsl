// Brightness threshold for bloom extraction
float brightness_threshold = 1.0f;
float bloom_intensity = 1.0f;
float bloom_blur_scale = 1.0f;

// Texture Sampler
Texture2D scene_texture : register(t0);    // Original scene texture
Texture2D bloom_texture : register(t1);    // For intermediate bloom results (brightness extraction, blur)
SamplerState tex_sampler : register(s0);   // Sampler for textures

// Viewport size and inverse
float2 viewport_size;
float2 inv_viewport_size;

// ---------------------------------------------------
// Step 1: Brightness Extraction (Filter bright areas)
// ---------------------------------------------------
float4 brightness_filter(float4 color) {
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));  // Luminance calculation
    if (brightness > brightness_threshold) {
        return color;  // Pass through bright areas
    } else {
        return float4(0.0, 0.0, 0.0, 1.0);  // Darken everything else
    }
}

// ---------------------------------------------------
// Step 2: Gaussian Blur (Two-pass, horizontal/vertical)
// ---------------------------------------------------
float4 gaussian_blur(float2 uv, Texture2D tex, float2 direction) {
    float weights[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};
    float4 result = tex.Sample(tex_sampler, uv) * weights[0];

    for (int i = 1; i < 5; i++) {
        result += tex.Sample(tex_sampler, uv + direction * inv_viewport_size * float(i)) * weights[i];
        result += tex.Sample(tex_sampler, uv - direction * inv_viewport_size * float(i)) * weights[i];
    }

    return result;
}

// ---------------------------------------------------
// Step 3: Bloom Combine (Add blur to original scene)
// ---------------------------------------------------
float4 bloom_combine(float4 original_color, float4 bloom_color) {
    // Combine original scene color with the blurred bloom color
    return original_color + bloom_color * bloom_intensity;
}

// ---------------------------------------------------
// Pixel Shader: Brightness Filter Pass
// ---------------------------------------------------
float4 ps_brightness(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET {
    // Sample the scene texture
    float4 scene_color = scene_texture.Sample(tex_sampler, uv);
    
    // Apply brightness filter to extract bright areas
    return brightness_filter(scene_color);
}

// ---------------------------------------------------
// Pixel Shader: Horizontal Gaussian Blur Pass
// ---------------------------------------------------
float4 ps_gaussian_blur_h(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET {
    // Apply horizontal blur (direction is (1, 0))
    return gaussian_blur(uv, bloom_texture, float2(bloom_blur_scale, 0.0f));
}

// ---------------------------------------------------
// Pixel Shader: Vertical Gaussian Blur Pass
// ---------------------------------------------------
float4 ps_gaussian_blur_v(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET {
    // Apply vertical blur (direction is (0, 1))
    return gaussian_blur(uv, bloom_texture, float2(0.0f, bloom_blur_scale));
}

// ---------------------------------------------------
// Pixel Shader: Bloom Combine Pass
// ---------------------------------------------------
float4 ps_bloom_combine(float4 position : SV_POSITION, float2 uv : TEXCOORD) : SV_TARGET {
    // Sample the original scene color
    float4 scene_color = scene_texture.Sample(tex_sampler, uv);
    
    // Sample the blurred bloom color
    float4 bloom_color = bloom_texture.Sample(tex_sampler, uv);
    
    // Combine bloom effect with the original scene
    return bloom_combine(scene_color, bloom_color);
}
