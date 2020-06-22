#version 410 core
uniform sampler2D tex;

uniform float u_min_alpha = 0.002f;
uniform float u_max_alpha = 0.3f;

in vec2 UV;
out vec4 color;

void main()
{
	vec4 c = texture2D(tex, UV);
	float lum = dot(c.rgb, vec3(0.299, 0.587, 0.114)); //luminosity
	color = vec4(c.rgb, u_min_alpha + ((u_max_alpha-u_min_alpha)*lum*lum*c.a));
}