#version 130 core

in vec4 worldPos;

out vec4 fColor;

const vec3 lightDir = const vec3(0.f, -1.f, 1.f);

const vec3 albedo = const vec3(1.f, 0.f, 0.f);

void main(){
	vec3 dx = vec3(dFdx(worldPos));
	vec3 dy = vec3(dFdy(worldPos));
	vec3 norm = normalize(vec3(cross(dx, dy)));

	fColor = vec4(norm.xyz * 0.5 + 0.5, 1.0);
	//fColor = vec4(albedo * dot(-lightDir, norm), 1.0);
}