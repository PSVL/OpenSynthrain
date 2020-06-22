#version 430 core

layout (points) in;
layout (triangle_strip, max_vertices = 32) out;
uniform float u_size = 0.02;
uniform float uv_spread = 0.001;
uniform float u_aspect = 1;
out vec2 UV;

const uint num_offests = 32;


void build_hex(vec4 position)
{    
	float y  = 0.5 * sqrt(3)*u_aspect;

	vec2 UV_center = (position.xy/2) + vec2(0.5,0.5);

	for(uint i = 0; i < num_offests; ++i)
	{
		gl_Position = position + vec2(sin(i), cos(i));
		UV = UV_center + uv_spread*offsets[i].xy;
		EmitVertex();
	}

    EndPrimitive();
}

void main()
{
	build_hex(vec4(gl_in[0].gl_Position.xy,0,1));
}


/*
	gl_Position = position + u_size*vec4( -1.0, 0.0, 0.0, 0.0);  
	UV = UV_center + uv_spread*vec2( -1.0, 0.0);
    EmitVertex();

    gl_Position = position + u_size*vec4(-0.5, y, 0.0, 0.0); 
    EmitVertex();

    gl_Position = position + u_size*vec4(-0.5, -y, 0.0, 0.0); 
    EmitVertex();   

	gl_Position = position + u_size*vec4( 0.5,  y, 0.0, 0.0);  
    EmitVertex();
	
    gl_Position = position + u_size*vec4( 0.5,  -y, 0.0, 0.0);  
    EmitVertex();

    gl_Position = position + u_size* vec4( 1.0,  0.0, 0.0, 0.0); 
    EmitVertex();
	*/
