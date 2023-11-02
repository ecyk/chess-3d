#version 330 core

out vec4 frag_color;

in vec3 frag_pos;
in vec3 normal;
in vec2 tex_coord;

uniform sampler2D base_tex;

uniform vec3 light_pos;
uniform vec3 view_pos;

const float ambient_strength = 0.25;
const float specular_strength = 0.75;

void main() {
    vec3 diffuse_tex = vec3(texture(base_tex, tex_coord));

    vec3 ambient = ambient_strength * diffuse_tex;

    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(light_pos - frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * diffuse_tex;

    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
    vec3 specular = specular_strength * spec * diffuse_tex;

    frag_color = vec4(ambient + diffuse + specular, 1.0);
}
