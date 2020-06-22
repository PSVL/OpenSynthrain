#version 410 core
uniform sampler2D tex;
uniform sampler2D billboard_tex;

uniform float u_alpha_min = 0.002f;
uniform float u_alpha_max = 0.3f;

in vec2 UV;
in vec2 UV2;
out vec4 color;
out vec4 ground_truth;

void main()
{
	vec4 bokeh = texture2D(billboard_tex, UV2);

	vec4 c = texture2D(tex, UV);
	float lum = bokeh.r * dot(c.rgb, vec3(0.299, 0.587, 0.114));// smoothstep( 0.9f, 1.0f, ) );
	float alpha = bokeh.a *c.a* (u_alpha_min + ((u_alpha_max-u_alpha_min)*lum));

	ground_truth = vec4(alpha*0.25,alpha*0.25,alpha*0.25,1);
	color = vec4(c.rgb, alpha);
}