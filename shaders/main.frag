#version 130

uniform usampler2D texture;
varying highp vec2 texc;

uniform int textureHeight;
uniform int textureWidth;

void main(void) {
    uint x = uint(texc.x * uint(textureWidth - 1)) / 2u;
    uint y = uint(texc.y * uint(textureHeight - 1)) / 2u;

    // HACK for passing 16-bit RGB via a byte buffer...
    uint index = texelFetch(texture, ivec2(2u * x, y), 0).r;
    float grayScale = 1.0 - index/3.0;
    gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
}
