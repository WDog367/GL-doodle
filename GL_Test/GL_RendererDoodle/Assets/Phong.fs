#version 330

in VsOutFsIn {
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
} fs_in;


out vec4 fragColour;

struct Material {
    vec3 kd;
    vec3 ks;
    float shininess;
};
uniform Material material;

const int lightNum = 20;

struct LightSource {
    vec3 position;
    vec3 colour;
    float attenuation[3];
};
uniform LightSource light[lightNum];
uniform int activeLights;

// Ambient light intensity for each RGB component.
uniform vec3 ambientIntensity;


vec3 phongModel(vec3 fragPosition, vec3 fragNormal) {


    // Direction from fragment to viewer (origin - fragPosition).
    vec3 v = normalize(-fragPosition.xyz);

    int top = min(lightNum, activeLights);

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    
    for (int i = 0; i < top; i++ ) {
        LightSource light = light[i];

        // Direction from fragment to light source.
        vec3 l = light.position - fragPosition;
        float d = length(l);
        l = l / d;

        float falloff = light.attenuation[0] + d * light.attenuation[1] + d*d* light.attenuation[2];
        vec3 rgbIntensity = light.colour / falloff;

        float n_dot_l = max(dot(fragNormal, l), 0.0);

        diffuse += rgbIntensity * n_dot_l;

        if (n_dot_l > 0.0) {
            // Halfway vector.
            vec3 h = normalize(v + l);
            float n_dot_h = max(dot(fragNormal, h), 0.0);

            specular += rgbIntensity * pow(n_dot_h, material.shininess);
        }
    }

    return ambientIntensity + (material.kd * diffuse + material.ks * specular);
}

void main() {
	fragColour = vec4(phongModel(fs_in.position_ES, normalize(fs_in.normal_ES)), 1.0);
}
