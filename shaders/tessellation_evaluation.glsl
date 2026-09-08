#version 460 core
layout(quads, fractional_odd_spacing, ccw) in;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D heightMap;
uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform float u_amplitude;
in vec2 TextureCoords[];

out VS_OUT {
  vec2 TexCoords;
  float Height;
  vec3 Normal;
  vec3 TangentFragPos;
  vec3 TangetLightDir;
  vec3 TangetViewPos;
  vec3 WorldPos;
  vec3 WorldNormal;
} tes_out;

void main() {
  float u = gl_TessCoord.x;
  float v = gl_TessCoord.y;

  vec2 t0 = (TextureCoords[1] - TextureCoords[0]) * u + TextureCoords[0];
  vec2 t1 = (TextureCoords[3] - TextureCoords[2]) * u + TextureCoords[2];
  tes_out.TexCoords = (t1 - t0) * v + t0;

  tes_out.Height = texture(heightMap, tes_out.TexCoords).r * u_amplitude;
  if (tes_out.Height < 0)
    tes_out.Height = 0;

  vec2 texelSize = vec2(1.0) / vec2(textureSize(heightMap, 0));
  float hL = texture(heightMap, tes_out.TexCoords - vec2(texelSize.x, 0.0)).r * u_amplitude;
  float hR = texture(heightMap, tes_out.TexCoords + vec2(texelSize.x, 0.0)).r * u_amplitude;
  float hD = texture(heightMap, tes_out.TexCoords - vec2(0.0, texelSize.y)).r * u_amplitude;
  float hU = texture(heightMap, tes_out.TexCoords + vec2(0.0, texelSize.y)).r * u_amplitude;

  tes_out.Normal = normalize(vec3(hL - hR, 2.0, hD - hU));
  if (tes_out.Height == 0)
    tes_out.Normal = vec3(0.0, 1.0, 0.0);

  mat3 normalMatrix = transpose(inverse(mat3(model)));
  vec3 N = normalize(normalMatrix * tes_out.Normal);
  vec3 T = normalize(normalMatrix * vec3(1.0, 0.0, 0.0));
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);
  mat3 TBN = transpose(mat3(T, B, N));

  tes_out.TangetLightDir = normalize(TBN * u_lightDir);
  tes_out.TangetViewPos = TBN * u_viewPos;
  tes_out.WorldNormal = N; // already world-space from normalMatrix transform

  vec4 p0 = (gl_in[1].gl_Position - gl_in[0].gl_Position) * u + gl_in[0].gl_Position;
  vec4 p1 = (gl_in[3].gl_Position - gl_in[2].gl_Position) * u + gl_in[2].gl_Position;
  vec4 p = (p1 - p0) * v + p0;
  p.y += tes_out.Height;

  vec3 FragPos = vec3(model * p);
  tes_out.WorldPos = FragPos;
  tes_out.TangentFragPos = TBN * FragPos;

  gl_Position = projection * view * model * p;
}
