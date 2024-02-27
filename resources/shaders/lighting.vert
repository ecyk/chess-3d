#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;

out vec3 world_pos;
out vec3 normal;
out vec2 tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat3 normal_mat;

void main() {
    world_pos = vec3(model * vec4(a_position, 1.0));
    gl_Position = projection * view * vec4(world_pos, 1.0);
    normal = normal_mat * a_normal;
    tex_coord = a_tex_coord;
}
