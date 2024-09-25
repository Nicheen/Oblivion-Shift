#define DETAIL_TYPE_ROUNDED_CORNERS 1
#define DETAIL_TYPE_OUTLINED 2
#define DETAIL_TYPE_OUTLINED_CIRCLE 3

// With custom shading we can extend the rendering library!
Draw_Quad *draw_rounded_rect(Vector2 p, Vector2 size, Vector4 color, float radius);
Draw_Quad *draw_rounded_rect_xform(Matrix4 xform, Vector2 size, Vector4 color, float radius);
Draw_Quad *draw_outlined_rect(Vector2 p, Vector2 size, Vector4 color, float line_width_pixels);
Draw_Quad *draw_outlined_rect_xform(Matrix4 xform, Vector2 size, Vector4 color, float line_width_pixels);
Draw_Quad *draw_outlined_circle(Vector2 p, Vector2 size, Vector4 color, float line_width_pixels);
Draw_Quad *draw_outlined_circle_xform(Matrix4 xform, Vector2 size, Vector4 color, float line_width_pixels);

Vector2 world_to_screen(Vector2 p) {
    Vector4 in_cam_space  = m4_transform(draw_frame.camera_xform, v4(p.x, p.y, 0.0, 1.0));
    Vector4 in_clip_space = m4_transform(draw_frame.projection, in_cam_space);
    
    Vector4 ndc = {
        .x = in_clip_space.x / in_clip_space.w,
        .y = in_clip_space.y / in_clip_space.w,
        .z = in_clip_space.z / in_clip_space.w,
        .w = in_clip_space.w
    };
    
    return v2(
        (ndc.x + 1.0f) * 0.5f * (f32)window.width,
        (ndc.y + 1.0f) * 0.5f * (f32)window.height
    );
}

Vector2 world_size_to_screen_size(Vector2 s) {
    Vector2 origin = v2(0, 0);
    
    Vector2 screen_origin = world_to_screen(origin);
    Vector2 screen_size_point = world_to_screen(s);
    
    return v2(
        screen_size_point.x - screen_origin.x,
        screen_size_point.y - screen_origin.y
    );
}

Draw_Quad *draw_rounded_rect(Vector2 p, Vector2 size, Vector4 color, float radius) {
	Draw_Quad *q = draw_rect(p, size, color);
	// detail_type
	q->userdata[0].x = DETAIL_TYPE_ROUNDED_CORNERS;
	// corner_radius
	q->userdata[0].y = radius;
	return q;
}
Draw_Quad *draw_rounded_rect_xform(Matrix4 xform, Vector2 size, Vector4 color, float radius) {
	Draw_Quad *q = draw_rect_xform(xform, size, color);
	// detail_type
	q->userdata[0].x = DETAIL_TYPE_ROUNDED_CORNERS;
	// corner_radius
	q->userdata[0].y = radius;
	return q;
}
Draw_Quad *draw_outlined_rect(Vector2 p, Vector2 size, Vector4 color, float line_width_pixels) {
	Draw_Quad *q = draw_rect(p, size, color);
	// detail_type
	q->userdata[0].x = DETAIL_TYPE_OUTLINED;
	// line_width_pixels
	q->userdata[0].y = line_width_pixels;
	// rect_size
	q->userdata[0].zw = world_size_to_screen_size(size);
	return q;
}
Draw_Quad *draw_outlined_rect_xform(Matrix4 xform, Vector2 size, Vector4 color, float line_width_pixels) {
	Draw_Quad *q = draw_rect_xform(xform, size, color);
	// detail_type
	q->userdata[0].x = DETAIL_TYPE_OUTLINED;
	// line_width_pixels
	q->userdata[0].y = line_width_pixels;
	// rect_size
	q->userdata[0].zw = world_size_to_screen_size(size);
	return q;
}
Draw_Quad *draw_outlined_circle(Vector2 p, Vector2 size, Vector4 color, float line_width_pixels) {
	Draw_Quad *q = draw_rect(p, size, color);
	// detail_type
	q->userdata[0].x = DETAIL_TYPE_OUTLINED_CIRCLE;
	// line_width_pixels
	q->userdata[0].y = line_width_pixels;
	// rect_size_pixels
	q->userdata[0].zw = world_size_to_screen_size(size); // Transform world space to screen space
	return q;
}
Draw_Quad *draw_outlined_circle_xform(Matrix4 xform, Vector2 size, Vector4 color, float line_width_pixels) {
	Draw_Quad *q = draw_rect_xform(xform, size, color);
	// detail_type
	q->userdata[0].x = DETAIL_TYPE_OUTLINED_CIRCLE;
	// line_width_pixels
	q->userdata[0].y = line_width_pixels;
	// rect_size_pixels
	q->userdata[0].zw = world_size_to_screen_size(size); // Transform world space to screen space
	
	return q;
}
