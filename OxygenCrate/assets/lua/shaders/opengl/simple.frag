#version 330 core
in vec3 v_Color;
uniform float u_Time;
out vec4 FragColor;
void main() {
    float pulseRaw = 0.5 + 0.5 * sin(u_Time);
    float pulse = 0.25 + 0.75 * pulseRaw;
    FragColor = vec4(v_Color * pulse, 1.0);
}
