#version 130

uniform usampler2D texture;
varying highp vec2 texc;

uniform int textureHeight;
uniform int textureWidth;

void main(void) {
    uint x = uint(texc.x * uint(textureWidth - 1)) / 2u;
    uint y = uint(texc.y * uint(textureHeight - 1)) / 2u;

    // HACK for passing 16-bit RGB via a byte buffer...
    uint lsb = texelFetch(texture, ivec2(2u * x, y), 0).r;
    uint msb = texelFetch(texture, ivec2(2u * x + 1u, y), 0).r;
    uint color = (msb << 8) | lsb;

    if ((color & (1u << 15u)) != 0u) {
        float grayScale = 1.0 - (color & 0x3u)/3.0;
        gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
    } else {
        float r = (color & 0x1fu)/31.0;
        float g = ((color >> 5) & 0x1fu)/31.0;
        float b = ((color >> 10) & 0x1fu)/31.0;
        gl_FragColor = vec4(r, g, b, 1.0);
    }
}
