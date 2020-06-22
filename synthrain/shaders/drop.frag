#version 410 core
uniform sampler2D drop_tex;

in vec2 UV;
in float depth;
out vec4 color;

void main()
{
	color = texture2D(drop_tex, UV);
	color.rgb *= color.a;
}