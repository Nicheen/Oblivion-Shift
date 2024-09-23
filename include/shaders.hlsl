
// PS_INPUT is defined in the default shader in gfx_impl_d3d11.c at the bottom of the file

// BEWARE std140 packing:
// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
cbuffer some_cbuffer : register(b0) {
    float2 mouse_pos_screen; // In pixels
    float2 window_size;
}

#define DETAIL_TYPE_ROUNDED_CORNERS 1
#define DETAIL_TYPE_OUTLINED 2
#define DETAIL_TYPE_OUTLINED_CIRCLE 3

float4 get_light_contribution(PS_INPUT input, float4 color) {
    // Calculate the brightness (luminance) of the color
    float brightness = dot(color.rgb, float3(0.299, 0.587, 0.114));

    // Amplify brightness to make bright pixels light up more
    float light_intensity = saturate(brightness * 200.0); // Adjust 2.0 for more or less intensity
    
    // Return the color multiplied by the light intensity (keeping the original color's alpha)
    return float4(color.rgb * light_intensity, color.a);
}


float4 extract_bright_areas(PS_INPUT input, float4 color) {
    // Calculate brightness (luminance) of the pixel
    float brightness = dot(color.rgb, float3(0.299, 0.587, 0.114));

    // Apply a threshold to select only bright areas
    float threshold = 0.8; // Adjust this to control the brightness level for bloom
    if (brightness < threshold) {
        return float4(0.0, 0.0, 0.0, 1.0); // Discard non-bright areas
    }

    // Return only the bright parts
    return color;
}

float4 horizontal_blur(PS_INPUT input, float4 color) {
    float4 result = float4(0, 0, 0, 0);
    float2 texelSize = float2(1.0 / window_size.x, 1.0 / window_size.y);
    float kernel[5] = {0.227027, 0.316216, 0.070270, 0.070270, 0.316216};  // Gaussian kernel
    float offset[5] = {-2.0, -1.0, 0.0, 1.0, 2.0};  // Pixel offsets for blur

    // Apply horizontal blur by sampling neighboring pixels in the horizontal direction
    for (int i = 0; i < 5; i++) {
        // Offset UV coordinates for horizontal blur
        float2 uv_offset = input.uv + float2(offset[i] * texelSize.x, 0);

        // Simulate sampling neighboring colors (using current color scaled by the kernel)
        float4 sample_color = color * kernel[i];  // Approximating blur by using current color scaled by the kernel

        // Accumulate the weighted color into the result
        result += sample_color;
    }

    return result;
}


float4 vertical_blur(PS_INPUT input, float4 color) {
    float4 result = float4(0, 0, 0, 0);
    float2 texelSize = float2(1.0 / window_size.x, 1.0 / window_size.y);
    float kernel[5] = {0.227027, 0.316216, 0.070270, 0.070270, 0.316216};  // Gaussian kernel
    float offset[5] = {-2.0, -1.0, 0.0, 1.0, 2.0};  // Pixel offsets for blur

    // Apply vertical blur by sampling neighboring pixels in the vertical direction
    for (int i = 0; i < 5; i++) {
        // Offset UV coordinates for vertical blur
        float2 uv_offset = input.uv + float2(0, offset[i] * texelSize.y);

        // Simulate sampling neighboring colors (in your case, you'd typically sample from a texture)
        float4 sample_color = color * kernel[i];  // Approximating blur by using current color scaled by the kernel

        // Accumulate the weighted color into the result
        result += sample_color;
    }

    return result;
}


// This procedure is the "entry" of our extension to the shader
// It basically just takes in the resulting color and input from vertex shader, for us to transform it
// however we want.
float4 pixel_shader_extension(PS_INPUT input, float4 color) {
    
    float detail_type = input.userdata[0].x;

    // Assumes rect with 90deg corners    
    float2 rect_size_pixels = input.userdata[0].zw;
    
	if (detail_type == DETAIL_TYPE_ROUNDED_CORNERS) {
		float corner_radius = input.userdata[0].y;
	
        float2 pos = input.self_uv - float2(0.5, 0.5);
	
		float2 corner_distance = abs(pos) - (float2(0.5, 0.5) - corner_radius);
		
		float dist = length(max(corner_distance, 0.0)) - corner_radius;
		float smoothing = 0.01;
		float mask = 1.0-smoothstep(0.0, smoothing, dist);
	
		color *= mask;
    } else if (detail_type == DETAIL_TYPE_OUTLINED) {
    	float line_width_pixels = input.userdata[0].y;
    	
    	float2 pixel_pos = round(input.self_uv*rect_size_pixels);
        
        float xcenter = rect_size_pixels.x/2;
        float ycenter = rect_size_pixels.y/2;
        
        float xedge = pixel_pos.x < xcenter ? 0.0 : rect_size_pixels.x;
        float yedge = pixel_pos.y < ycenter ? 0.0 : rect_size_pixels.y;
        
        float xdist = abs(xedge-pixel_pos.x);
        float ydist = abs(yedge-pixel_pos.y);
        
        if (xdist > line_width_pixels && ydist > line_width_pixels) {
            discard;
        }
    } else if (detail_type == DETAIL_TYPE_OUTLINED_CIRCLE) {
        float line_width_pixels = input.userdata[0].y;
        float2 rect_size_pixels = input.userdata[0].zw;
        float line_width_uv = line_width_pixels / min(rect_size_pixels.x, rect_size_pixels.y);
        
        // For some simple anti-aliasing, we add a little bit of padding around the outline
        // and fade that padding outwards with a smooth curve towards 0. 
        // Very arbitrary smooth equation that I got from just testing different sizes of the circle.
        // It's kinda meh.
        float smooth = ((4.0/line_width_pixels)*6.0)/window_size.x;
    
        float2 center = float2(0.5, 0.5);
        float dist = length(input.self_uv - center);
        
        float mask;
        if (dist > 0.5-smooth) {
            mask = 1.0-lerp(0, 1.0, max(dist-0.5+smooth, 0.0)/smooth);
        } else if (dist < 0.5-line_width_uv+smooth) {
            mask = smoothstep(0, 1.0, max(dist-0.5+line_width_uv+smooth, 0.0)/smooth);
        }
        if (mask <= 0) discard;
        color *= mask;
	}
    // Makes a light around bright objects
	// float4 light = get_light_contribution(input, color);

    // --- Bloom effect ---
    float4 bright_areas = extract_bright_areas(input, color);

    float4 blurred_horiz = horizontal_blur(input, color);
    float4 blurred_vert = vertical_blur(input, blurred_horiz);

    float4 bloom = blurred_vert * bright_areas;

    // color += bloom * 10;

    return color;
}