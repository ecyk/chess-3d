#version 330 core

out vec4 frag_color;

uniform vec4 color;

void main() {
    frag_color = color;
}
