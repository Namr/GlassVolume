#version 400 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec3 vray_dir;
flat in vec3 transformed_eye;

uniform sampler3D volumeTexture;
uniform sampler1D transfer_fcn;
uniform ivec3 volume_dims;
uniform mat4 model;

out vec4 outColor;

vec2 intersect_box(vec3 orig, vec3 dir) {
	const vec3 box_min = vec3(0);
	const vec3 box_max = vec3(1);
	vec3 inv_dir = 1.0 / dir;
	vec3 tmin_tmp = (box_min - orig) * inv_dir;
	vec3 tmax_tmp = (box_max - orig) * inv_dir;
	vec3 tmin = min(tmin_tmp, tmax_tmp);
	vec3 tmax = max(tmin_tmp, tmax_tmp);
	float t0 = max(tmin.x, max(tmin.y, tmin.z));
	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	return vec2(t0, t1);
}

void main(void) {
	// Step 1: Normalize the view ray
	vec3 ray_dir = normalize(vray_dir);


	// Step 3: Compute the step size to march through the volume grid
	vec3 dt_vec = 1.0 / (vec3(volume_dims) * abs(ray_dir));
	float dt = min(dt_vec.x, min(dt_vec.y, dt_vec.z));

	// Step 4: Starting from the entry point, march the ray through the volume
	// and sample it
	vec3 p = FragPos;
	while(true) {
		vec3 rotP = vec3(inverse(model) * vec4(p, 1.0)) + vec3(0.5);
		// Step 4.1: Sample the volume, and color it by the transfer function.
		// Note that here we don't use the opacity from the transfer function,
		// and just use the sample value as the opacity
		float val = texture(volumeTexture, rotP).r;
		vec4 val_color = vec4(val);
		//if(p.x > 0.5)
			val_color = vec4(texture(transfer_fcn, val).rgb, val);

		// Step 4.2: Accumulate the color and opacity using the front-to-back
		// compositing equation
		outColor.rgb += (1.0 - outColor.a) * val_color.a * val_color.rgb;
		outColor.a += (1.0 - outColor.a) * val_color.a;

		// Optimization: break out of the loop when the color is near opaque
		if (outColor.a >= 0.95) {
			//break;
		}

		if(max(rotP.x, max(rotP.y, rotP.z)) > 1.1 || min(rotP.x, min(rotP.y, rotP.z)) < -0.1)
		{
			break;
		}
		p += ray_dir * dt;
	}

	//outColor = vec4(1.0, 0.0, 0.0, 1.0);
}