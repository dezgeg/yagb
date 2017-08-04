#version 130

uniform sampler2D texture;
uniform sampler2D xAxisGrid;
uniform sampler2D yAxisGrid;
varying highp vec2 texc;

uniform int bgPatternBaseSelect;
uniform int bgTileBaseSelect;

uniform int textureHeight;
uniform int textureWidth;

void main(void) {
    int patternOff = bgPatternBaseSelect != 0 ? 0x0 : 0x1000;
    int tileOff = bgTileBaseSelect != 0 ? 0x1c00 : 0x1800;

    int x = int(round(texc.x * (textureWidth - 1)));
    int y = int(round(texc.y * (textureHeight - 1)));

    int tileX = x / 17;
    int tileY = y / 17;
    int bitX = (x % 17) / 2;
    int bitY = (y % 17) / 2;

    int rawTile = int(round(texture2D(texture, vec2((tileOff + 32 * tileY + tileX) / 8191.0, 0.0)).r * 255.0));
    int tile = (bgPatternBaseSelect != 0) ? rawTile : (rawTile < 128 ? rawTile : -(256 - rawTile));
    int offs = patternOff + 16 * tile;

    int lsbs = int(texelFetch(texture, ivec2((offs + 2 * bitY), 0.0), 0).r * 255.0);
    int msbs = int(texelFetch(texture, ivec2((offs + 2 * bitY + 1), 0.0), 0).r * 255.0);

    int b0 = (lsbs >> (7 - bitX)) & 1;
    int b1 = (msbs >> (7 - bitX)) & 1;

    float grayScale = 1.0 - float(2 * b1 + b0)/3.0;

    if (x % 17 == 16 || y % 17 == 16) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
    }
}
