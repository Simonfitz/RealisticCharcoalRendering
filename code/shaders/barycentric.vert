#version 330
//Inputs
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normals;
layout(location = 2) in vec2 tex_coords;
layout(location = 3) in vec3 vertex_tangent;
layout(location = 4) in vec3 vertex_bitangent;

//Outputs
out vec3 eye_normal;
out vec3 V;
out vec4 light;
out vec2 texture;
out vec3 tangent;
out vec3 bitangent;

out vec4 pos_light_space;

//Uniforms
uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;
uniform mat4 lightMVP;
uniform vec4 light_pos;
uniform vec4 light_dir;


void main()
{
	light = (view * vec4(light_pos.xyz, 0.0));
	V = normalize((view * model * vec4(vertex_position, 1.0)).xyz);
	eye_normal = normalize(view * model * vec4 (vertex_normals, 0.0)).xyz;
	gl_Position =  proj * view * model * vec4 (vertex_position, 1.0);
	
	pos_light_space = lightMVP * model * vec4(vertex_position, 1.0);
	
	texture = tex_coords;
}