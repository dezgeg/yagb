uniform sampler2D texture;
varying highp vec2 texc;

void main(void) {
    float x = (texc.x - 0.375) * (271.0 - 1.0);
    float y = (texc.y - 0.375) * (407.0 - 1.0);

    if (int(floor(mod(x, 17.0))) == 0 || int(floor(mod(y, 17.0))) == 0)
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else
        gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);
}
