#version 430 core

in vec4 worldPos;

out vec4 fColor;

const vec3 lightDir = const vec3(-1.f, -1.f, 1.f);

uniform mat4 cameraMatrix;

uniform vec3 diffuse;
uniform vec3 specular;
uniform vec3 emissive;

uniform float shininess;

void main(){
	vec3 dx = vec3(dFdx(worldPos));
	vec3 dy = vec3(dFdy(worldPos));
	vec3 norm = normalize(vec3(cross(dx, dy)));

	vec3 v = normalize(vec3(transpose(cameraMatrix)[3]) - worldPos.xyz);

	vec3 l = -normalize(lightDir);
	vec3 h = normalize(l + v);

	vec3 d = diffuse * clamp(dot(l, norm), 0.0, 1.0);
	vec3 s = specular * pow(clamp(dot(norm, h), 0.0, 1.0), shininess);
	vec3 e = emissive;


	fColor = vec4(d + s + e, 1.0);
}