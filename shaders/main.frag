#version 130

uniform usampler2D texture;
varying highp vec2 texc;

uniform int textureHeight;
uniform int textureWidth;

void main(void) {
    uint x = uint(round(texc.x / 2 * uint(textureWidth - 1)));
    uint y = uint(round(texc.y / 2 * uint(textureHeight - 1)));

    uint index = texelFetch(texture, ivec2(x, y), 0).r;
    float grayScale = 1.0 - index/3.0;
    gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
}
