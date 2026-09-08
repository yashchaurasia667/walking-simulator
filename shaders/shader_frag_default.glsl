#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D heightMap;

void main() {
  float h = (texture(heightMap, TexCoords).r + 1) / 2;
  FragColor = vec4(h, h, h, 1.0);

  // FragColor = texture(heightMap, TexCoords);
}
