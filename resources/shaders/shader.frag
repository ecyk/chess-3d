#version 330 core

out vec4 frag_color;

in vec2 tex_coord;

uniform sampler2D base_texture;
uniform vec4 solid_color;

uniform float blend_factor;

void main() {
    frag_color = mix(solid_color, texture(base_texture, tex_coord), blend_factor);
}
