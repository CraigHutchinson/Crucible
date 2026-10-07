#version 450
layout(location=0) in vec2 quad;
layout(location=1) in vec4 geometry;
layout(location=2) in vec4 opaqueSrgb;
layout(set=1,binding=0,std140) uniform Camera {
    vec4 originExtent;
    vec4 canvasMarker;
} camera;
layout(location=0) out vec4 color;
void main() {
    vec2 center = camera.originExtent.xy + geometry.xy * camera.originExtent.zw;
    vec2 halfExtent = camera.canvasMarker.w > 0.5
        ? vec2(camera.canvasMarker.z) : geometry.zw * camera.originExtent.zw;
    vec2 logical = clamp(center + quad * halfExtent, vec2(24.0,96.0), vec2(1256.0,616.0));
    gl_Position = vec4(2.0*logical.x/camera.canvasMarker.x-1.0,
                       1.0-2.0*logical.y/camera.canvasMarker.y,0.0,1.0);
    color = opaqueSrgb;
}
