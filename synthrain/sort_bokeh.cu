#include <GL/glew.h> 
#include <GL/gl.h> 
#include <cuda_gl_interop.h> 
#include <thrust/device_ptr.h>
#include <thrust/sort.h>

extern "C" void sort_pixels(size_t num_pixels);
extern "C" void register_buffer(GLuint buffer);

static GLuint  bufferObj;
static cudaGraphicsResource *resource;

struct sort_functor
{
	__host__ __device__
	bool operator()(float4 left, float4 right) const
	{
		return (left.z < right.z);
	}
};

extern "C"
void sort_pixels(size_t num_pixels) {
	cudaGraphicsMapResources(1, &resource, NULL);
	float4* devPtr;
	size_t  size;

	cudaGraphicsResourceGetMappedPointer((void**)&devPtr, &size, resource);
	thrust::device_ptr<float4> tptr = thrust::device_pointer_cast(devPtr);
	thrust::sort(tptr, tptr + (num_pixels), sort_functor());
	cudaGraphicsUnmapResources(1, &resource, NULL);
}

extern "C"
void register_buffer(GLuint buffer)
{
	bufferObj = buffer;
	cudaGraphicsGLRegisterBuffer(&resource, bufferObj, cudaGraphicsMapFlagsNone);
}
