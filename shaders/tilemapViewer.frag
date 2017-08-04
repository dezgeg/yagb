#version 130

uniform sampler2D texture;
uniform sampler2D xAxisGrid;
uniform sampler2D yAxisGrid;
varying highp vec2 texc;

uniform int bgPatternBaseSelect;
uniform int bgTileBaseSelect;

void main(void) {
    float xc = texture2D(xAxisGrid, vec2(texc.x, 0)).r;
    float yc = texture2D(yAxisGrid, vec2(texc.y, 0)).r;

#if 0
    int patternOff = bgPatternBaseSelect != 0 ? 0x0 : 0x1000;
    int tileOff = bgTileBaseSelect != 0 ? 0x1c00 : 0x1800;
#else
    int patternOff = (1 - bgPatternBaseSelect) * 0x1000;
    int tileOff = 0x1800 + (bgTileBaseSelect * 0x400);
#endif

    //int patternOff = true ? 0x0 : 0x1000;
    //int tileOff = false ? 0x1c00 : 0x1800;

    int tileX = int(texc.x * 32.0);
    int tileY = int(texc.y * 32.0);
    int bitX = int(round(xc * 7));
    int bitY = int(round(yc * 7));

    int rawTile = int(round(texture2D(texture, vec2((tileOff + 32 * tileY + tileX) / 8191.0, 0.0)).r * 255.0));
    int tile = (bgPatternBaseSelect != 0) ? rawTile : (rawTile < 128 ? rawTile : -(256 - rawTile));
    int offs = patternOff + 16 * tile;

    int lsbs = int(round(texture2D(texture, vec2((offs + 2 * bitY) / 8191.0, 0.0)).r * 255.0));
    int msbs = int(round(texture2D(texture, vec2((offs + 2 * bitY + 1) / 8191.0, 0.0)).r * 255.0));

    int b0 = (lsbs >> (7 - bitX)) & 1;
    int b1 = (msbs >> (7 - bitX)) & 1;

    float grayScale = 1.0 - float(2 * b1 + b0)/3.0;

    if (xc == 1.0 || yc == 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);

        //gl_FragColor = vec4(float(bgPatternBaseSelect), 0.0, float(bgTileBaseSelect), 1.0);
        //gl_FragColor = vec4(1.0, tileX / 31.0, tileY / 31.0, 1.0);
        //gl_FragColor = vec4(tile / 64.0, 0.0, 0.0, 1.0);
        //gl_FragColor = vec4(lsbs / 255.0, 0.0, 0.0, 1.0);
    }
}
