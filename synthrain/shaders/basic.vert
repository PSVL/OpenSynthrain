#version 410 core

layout(location = 0) in vec2 vtx_pos;

out float depth;
out vec2 UV;

void main()
{
  gl_Position.xy = (vtx_pos*2)+vec2(-1,-1);
  gl_Position.z = 1;
  gl_Position.w = 1.0;
  UV = vtx_pos;
}