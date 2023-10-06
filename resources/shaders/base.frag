#version 330 core

out vec4 FragColor;

in vec2 tex_coord;

uniform sampler2D texture_base;

void main() {
    FragColor = texture(texture_base, tex_coord);
}
