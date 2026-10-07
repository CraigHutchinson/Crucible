#version 450
layout(location=0) in vec4 color;
layout(location=0) out vec4 outputColor;
void main() {
    // RGBA8_UNORM stores these encoded sRGB values directly; alpha is always one.
    outputColor = color;
}
