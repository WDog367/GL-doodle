#version 430 core

layout(location = 0) in vec4 coord;
in vec3 normal;
out vec4 color;
out float z;

uniform mat4 mvp;
uniform mat3 m_3x3_inv_transp;
uniform int collision;

void main(void) {
  color = vec4(1.0, (collision == 0 ? 0.0 : 1.0) , 0.0, 1.0);

  vec3 normalDirection = normalize(m_3x3_inv_transp * normal);
  vec3 lightDirection = normalize(vec3(1, 1, 1));

  color = vec4(vec3(color)* max(0.0, dot(normalDirection, lightDirection)), 1.0);

  gl_Position = mvp*coord;
  gl_PointSize = 10;

  z = gl_Position.z;
}