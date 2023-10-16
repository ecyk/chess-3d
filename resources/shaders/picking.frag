#version 330 core

out int frag_color;

uniform int color;

void main() {
    frag_color = color;
}
