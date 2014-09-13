uniform sampler2D texture;
varying highp vec2 texc;

void main(void) {
    float index = texture2D(texture, texc).r * 255.0;
    float grayScale = 1.0 - index/3.0;
    gl_FragColor = vec4(grayScale, grayScale, grayScale, 1.0);
}
