#version 430 core
uniform sampler2D bg_tex;
uniform sampler2D drop_tex;

uniform vec2 u_texel = {1/1440.0f,1/1080.0f};

uniform float u_alphaMultiply = 10;
uniform float u_alphaSubtract = 1;
uniform float u_minRefraction = 2;
uniform float u_refractionDelta = 1.f;

in vec2 UV;
in float depth;
out vec4 color;

layout(binding = 1, offset = 0) uniform atomic_uint bokeh_count;

layout(std430, binding = 1) buffer bokeh_points
{
    vec4 bokeh[];
};


vec4 blend(vec4 bg,vec4 fg){
  vec3 bgm=bg.rgb*bg.a;
  vec3 fgm=fg.rgb*fg.a;
  float ia=1.0-fg.a;
  float a=(fg.a + bg.a * ia);
  vec3 rgb;
  if(a!=0.0){
    rgb=(fgm + bgm * ia) / a;
  }else{
    rgb=vec3(0.0,0.0,0.0);
  }
  return vec4(rgb,a);
}

vec3 filterNormal(sampler2D bilinearSampler, vec2 uv)
{
 vec3 h;
 vec3 v;
 h[0] = texture2D(bilinearSampler, uv + u_texel*vec2(-1, 0)).a;
 h[1] = texture2D(bilinearSampler, uv).a;
 h[2] = texture2D(bilinearSampler, uv + u_texel*vec2( 1, 0)).a;
 v[0] = texture2D(bilinearSampler, uv + u_texel*vec2( 0,-1)).a;
 v[1] = h[1];
 v[2] = texture2D(bilinearSampler, uv + u_texel*vec2( 0, 1)).a;
 vec3 n;
 n.z =((v[0] - v[1]) + (v[1] - v[2])) /2.0f;
 n.x = ((h[0] - h[1]) + (h[1] - h[2])) /2.0f;
 n.y = 2;

 return normalize(n);
} 


void main()
{
	vec4 dropmap = texture2D(drop_tex, UV);
	dropmap.rgb /= dropmap.a;
	float thickness = 0;
    vec2 refraction = (vec2(dropmap.g-0.5f, dropmap.r-0.5f));
	vec2 refractionPos = UV + ((u_minRefraction+(thickness*u_refractionDelta))-sin(dropmap.a*(3.14159265359f/2)))*refraction;//*refraction*refraction;
	float alpha = clamp(dropmap.a*u_alphaMultiply-u_alphaSubtract, 0.0, 1.0);

	vec4 drops =  vec4(texture2D(bg_tex, refractionPos).rgb, alpha);
	vec4 bg = texture2D(bg_tex, UV);

	color = vec4(drops.rgb, drops.a);
	float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
	if( alpha > 0)
	{
		uint index = atomicCounterIncrement(bokeh_count);
		index %= 256000;
		bokeh[index] = vec4(UV.xy, luma,dropmap.a);	
	}

	
}