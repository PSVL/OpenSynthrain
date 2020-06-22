#version 410 core
uniform sampler2D tex;
uniform float alpha = 0.1;

in vec2 UV;
out vec4 color;

void main()
{
	color =  vec4(texture2D(tex, UV).rgb, alpha);
}