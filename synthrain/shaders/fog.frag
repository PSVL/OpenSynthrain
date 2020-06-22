#version 450 core
layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D depth;
layout(binding = 2) uniform sampler2D blur;

uniform float density = 0.4;
uniform float far_fog_plane = 0.000f;
uniform float near_fog_plane = 0.063f;
uniform vec3 fog_color = {0.89,0.89,0.89};
uniform float alpha = 0.23;
uniform float distance_pow = 1;
uniform float fog_max_density = 0.91f;


in vec2 UV;
out vec4 color;


void main()
{
    vec4 blur_color =  vec4(texture2D(blur, UV).rgb, alpha);
	vec4 source_color =  vec4(texture2D(tex, UV).rgb, alpha);

	/*float z = clamp(texture2D(depth, UV).r,far_fog_plane,near_fog_plane);
	float z_scaled = (z-far_fog_plane)/(near_fog_plane-far_fog_plane);
	float distance =  distance_pow/(z_scaled);
	float fog = (1/exp((distance*density)));*/
	

	float distance = clamp(texture2D(depth, UV).r,far_fog_plane,near_fog_plane);
	float distance_scaled = log(1/((distance-far_fog_plane)/(near_fog_plane-far_fog_plane)));
	float fog = clamp(1-(1/exp((distance_scaled*density))),0.f,fog_max_density);

	color = vec4( mix(mix(blur_color.rgb,source_color.rgb,alpha),  fog_color,  fog), 1);
	//color = vec4(fog,0,0,1);
}