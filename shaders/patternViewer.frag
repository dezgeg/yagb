uniform sampler2D texture;
uniform sampler2D xAxisGrid;
uniform sampler2D yAxisGrid;
varying highp vec2 texc;

void main(void) {
    float xc = texture2D(xAxisGrid, vec2(texc.x, 0)).r;
    float yc = texture2D(yAxisGrid, vec2(texc.y, 0)).r;

    if (xc == 1.0 || yc == 1.0)
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
