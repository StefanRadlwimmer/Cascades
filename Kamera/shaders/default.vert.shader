#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;

out VS_OUT
{
	vec3 FragPos;
	vec3 Normal;
	vec2 UV;
	mat3 TBN;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view *  model * vec4(position, 1.0f);
	vs_out.FragPos = vec3(model * vec4(position, 1.0f));
	vs_out.UV = uv;
	vs_out.Normal = mat3(transpose(inverse(model))) * normal;

	vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
	vec3 B = cross(T, N);
	vs_out.TBN = mat3(T, B, N);
}