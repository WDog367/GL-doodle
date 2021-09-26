#version 330 core

in vec2 uv;

uniform sampler2D image;
uniform sampler2D progressImage;

uniform int width;
uniform int height;

const int MAX_RADIUS = 200;

void main()
{
	//gl_FragColor = texture(texture, uv);
	vec2 flipped_uv = vec2(1.0, -1.0) * uv;

	float texel_width = 1.0 / width;
	float texel_height = 1.0 / height;
	
	gl_FragColor = texture(image, flipped_uv);
	
	int found = 0;
	int mip_level = 0;
	int r = 1;
	
	vec4 color = vec4(0, 0, 0, 0);
	
	for (r = 1; r < MAX_RADIUS; r *= 2) {
		for (int x = 0; x < r; x++) {
			for (int y = 0; y < r; y++) {
				vec2 offset_uv = vec2(
				(((int(flipped_uv.x * width)) >> mip_level << mip_level) + x) * texel_width , 
				(((int(flipped_uv.y * height))>> mip_level << mip_level) + y) * texel_height
				);
			
				vec4 progress = texture(progressImage, offset_uv);
				if (vec3(progress) == vec3(1.0, 1.0, 1.0)) {
					gl_FragColor = texture(image, offset_uv);
					found += 1;
					color += (texture(image, offset_uv) - color) / float(found);
				}
			}
		}
		if (found > 0) {
			break;
		}
		mip_level += 1;
	}
	
	gl_FragColor = color;
	
#if 0
	// search for completed pixel in spiral pattern
	for (int r = 0; r < MAX_RADIUS; r++) {
		for (int x = -r; x <= r; x++) {
			for (int y = -r; y <= r; y++) {
				if (x != -r && x != r && y != -r && y != r) {
					continue;
				}
				vec2 offset_uv = flipped_uv + vec2(texel_width * x, texel_height * y);

				vec4 progress = texture(progressImage, offset_uv);
				if (vec3(progress) == vec3(1.0, 1.0, 1.0)) {
					gl_FragColor = texture(image, offset_uv);
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
		}
		if (found) {
			break;
		}
	}
#endif
}