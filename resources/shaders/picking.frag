#version 330 core

out vec4 FragColor;

uniform vec4 picking_color;

void main() {
    FragColor = picking_color;
}
