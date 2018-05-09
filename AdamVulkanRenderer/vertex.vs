#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) uniform bufferVals {
    mat4 mvp;
} myBufferVals;
layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 inTexCoords;
layout (location = 0) out vec2 texcoord;
layout (location = 1) out vec3 posColor;
out gl_PerVertex { 
    vec4 gl_Position;
};
void main() {
   // Vulkan top-left 0,0 like direct-x, so adjust coordinates
   texcoord = 1.0-inTexCoords;
   gl_Position = myBufferVals.mvp * pos;
   posColor = pos.xyz;
}
