#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float outline_thickness;

void main() {
    gl_Position = projection * view * model * vec4(a_position + a_normal * outline_thickness, 1.0f);
}
