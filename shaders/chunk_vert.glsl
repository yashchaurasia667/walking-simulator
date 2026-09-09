#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;

out vec2 TexCoords;

// out gl_PerVertex
// {
//   vec4 gl_Position;
//   float gl_PointSize;
//   float gl_ClipDistance[];
// } gl_in[gl_MaxPatchVertices];

void main() {
  gl_Position = vec4(aPos, 1.0);
  TexCoords = aTex;
}
