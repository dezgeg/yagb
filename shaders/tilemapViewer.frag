#version 130

uniform usampler2D texture;
varying highp vec2 texc;

uniform int bgPatternBaseSelect;
uniform int bgTileBaseSelect;

uniform int textureHeight;
uniform int textureWidth;

void main(void) {
    uint patternOff = bgPatternBaseSelect != 0 ? 0x0u : 0x1000u;
    uint tileOff = bgTileBaseSelect != 0 ? 0x1c00u : 0x1800u;

    uint x = uint(round(texc.x * uint(textureWidth - 1)));
    uint y = uint(round(texc.y * uint(textureHeight - 1)));

    uint tileX = x / 17u;
    uint tileY = y / 17u;
    uint bitX = (x % 17u) / 2u;
    uint bitY = (y % 17u) / 2u;

    uint rawTile = texelFetch(texture, ivec2(tileOff + 32u * tileY + tileX, 0.0), 0).r;
    int tile = (bgPatternBaseSelect != 0) ? int(rawTile) : (rawTile < 128u ? int(rawTile) : -(256 - int(rawTile)));
    uint offs = uint(int(patternOff) + 16 * tile);

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
