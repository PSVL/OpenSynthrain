#version 410 core

layout(location = 0) in vec2 vtx_pos;
layout(location = 1) in vec4 params;

out float depth;
out vec2 UV;

uniform vec2 inv_half_screen_size;

void main()
{
  gl_Position.xy = ( ((vtx_pos*params.zw)+params.xy) *inv_half_screen_size ) - vec2(1,1) ;
  gl_Position.z = 1;
  gl_Position.w = 1.0;
  UV = vtx_pos + vec2(0.5f,0.5f);
  depth = params.w;
}