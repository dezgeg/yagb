#version 130

uniform usampler2D texture;
varying highp vec2 texc;

uniform int textureHeight;
uniform int textureWidth;

void main(void) {
    uint x = uint(round(texc.x * (uint(textureWidth) - 1u)));
    uint y = uint(round(texc.y * (uint(textureHeight) - 1u)));

    uint tileX = x / 17u;
    uint tileY = y / 17u;
    uint bitX = (x % 17u) / 2u;
    uint bitY = (y % 17u) / 2u;

    uint offs = 16u * (16u * tileY + tileX);

    uint lsbs = texelFetch(texture, ivec2((offs + 2u * bitY), 0.0), 0).r;
    uint msbs = texelFetch(texture, ivec2((offs + 2u * bitY + 1u), 0.0), 0).r;

    uint b0 = (lsbs >> (7u - bitX)) & 1u;
    uint b1 = (msbs >> (7u - bitX)) & 1u;

    float grayScale = 1.0 - float(2u * b1 + b0)/3.0;

    if (x % 17u == 16u || y % 17u == 16u) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
    }
}
