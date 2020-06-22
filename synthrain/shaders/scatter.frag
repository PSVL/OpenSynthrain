#version 450 core
layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D depth;
uniform vec2 resolution = vec2(960,960);
uniform vec2 direction = vec2(1,0);

uniform float far_fog_plane = 0.007f;
uniform float near_fog_plane = 0.024f;
uniform float density = 1;
uniform float distance_pow = 1;

in vec2 UV;
out vec4 color;

vec4 blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.3846153846) * direction;
  vec2 off2 = vec2(3.2307692308) * direction;
  color += texture2D(image, uv) * 0.2270270270;
  color += texture2D(image, uv + (off1 / resolution)) * 0.3162162162;
  color += texture2D(image, uv - (off1 / resolution)) * 0.3162162162;
  color += texture2D(image, uv + (off2 / resolution)) * 0.0702702703;
  color += texture2D(image, uv - (off2 / resolution)) * 0.0702702703;
  return color;
}

void main()
{
	float distance = clamp(texture2D(depth, UV).r,far_fog_plane,near_fog_plane);
	float distance_scaled = log(1/((distance-far_fog_plane)/(near_fog_plane-far_fog_plane)));
	float fog = 1-(1/exp((distance_scaled*distance_scaled*density)));

	color = blur9(tex, UV, resolution, direction*fog);
//	color = vec4(direction*fog,0,1) ;

//	fog *= fog_color_scale;
//	color = vec4((1-fog)*color.rgb +  fog*fog_brightness*vec3(1,1,1),1);
}