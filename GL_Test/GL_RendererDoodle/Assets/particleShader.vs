#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 velocity;
layout(location = 2) in vec3 colour;
layout(location = 3) in float lifetime;

layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

out vec3 particleColour;

out vs_out_gs_in
{
  vec3 position;
  vec3 velocity;
  vec3 colour;
  float lifetime;

  vec3 tangent;
  vec3 bitangent;
} vs_out;


uniform mat4 ModelView;
uniform mat4 Perspective;

// Remember, this is transpose(inverse(ModelView)).  Normals should be
// transformed using this matrix instead of the ModelView matrix.
uniform mat3 NormalMatrix;

void main() {
    vs_out.position = position;
    vs_out.velocity = velocity;
    vs_out.colour = colour;
    vs_out.lifetime = lifetime;
    vs_out.tangent = tangent;
    vs_out.bitangent = bitangent;
}