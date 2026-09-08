#version 460 core

layout(local_size_x = 16, local_size_y = 16) in;

uniform int u_seed;
uniform vec2 u_chunkOffset;
uniform writeonly image2D u_heightMap;

uniform int u_cellWidth;
uniform int u_chunkWidth;
uniform int u_noisePass;

uniform float u_amplitude;
uniform float u_frequency;
uniform float u_slopeStrength;
uniform float u_lacunarity;
uniform float u_persistance;

vec3 directions[] = {
    vec3(1, 1, 0),
    vec3(-1, 1, 0),
    vec3(1, -1, 0),
    vec3(-1, -1, 0),
    vec3(1, 0, 1),
    vec3(-1, 0, 1),
    vec3(1, 0, -1),
    vec3(-1, 0, -1),
    vec3(0, 1, 1),
    vec3(0, 1, -1),
    vec3(0, -1, 1),
    vec3(0, -1, -1),
    vec3(1, 1, 0),
    vec3(-1, 1, 0),
    vec3(1, -1, 0),
    vec3(-1, -1, 0),
  };
int perm[256] = int[256](
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
    247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
    57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
    74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
    60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
    65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
    200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
    52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
    207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
    119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
    218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
    81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
    184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
    222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
  );

float lerp(float v0, float v1, float t);
float cerp(float v0, float v1, float t);
int pickGradient(int x, int y);
vec3 perlinD(vec2 pos);
vec3 fbm(vec2 pos);

void main() {
  ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
  ivec2 texSize = imageSize(u_heightMap);
  // if (texCoord.x >= u_chunkWidth || texCoord.y >= u_chunkWidth)
  //   return;

  vec2 worldPos = u_chunkOffset + vec2(texCoord);
  vec3 val = fbm(worldPos);
  imageStore(u_heightMap, texCoord, vec4(val, 1.0));
}

float lerp(float v0, float v1, float t) {
  return (1 - t) * v0 + t * v1;
}

float cerp(float v0, float v1, float t) {
  return (v1 - v0) * (3.0f - t * 2.0f) * t * t + v0;
}

int pickGradient(int x, int y) {
  int ix = (x ^ u_seed) & 255;
  int iy = (y ^ u_seed) & 255;
  return perm[(perm[ix] + iy) & 255] & 15;
}

vec3 fbm(vec2 pos) {
  float height = 0.0;
  float totalAmp = 0.0;
  float amp = 1.0;
  float freq = u_frequency;
  vec2 gradient = vec2(0.0);

  for (int i = 0; i < u_noisePass; i++) {
    vec3 nd = perlinD(pos * freq);
    // steeper accumulated gradient → less contribution from this octave
    float weight = 1.0 / (1.0 + dot(gradient, gradient) * u_slopeStrength);

    height += amp * nd.x * weight;
    totalAmp += amp;

    // accumulate gradient in world space
    // nd.yz is gradient in (pos*freq) space, multiply by freq to get world space
    gradient += amp * nd.yz;

    amp *= u_persistance;
    freq *= u_lacunarity;
  }
  return vec3(height / totalAmp, gradient);
  // float worldHeight = (height / totalAmp) * u_amplitude;
  // vec2 worldGrad = (gradient / totalAmp) * u_amplitude;
  // return vec3(worldHeight, worldGrad);
}

vec3 perlinD(vec2 pos) {
  ivec2 p0 = ivec2(floor(pos / float(u_cellWidth)) * float(u_cellWidth));
  ivec2 p1 = p0 + u_cellWidth;

  vec3 d00 = directions[pickGradient(p0.x, p0.y)];
  vec3 d01 = directions[pickGradient(p0.x, p1.y)];
  vec3 d10 = directions[pickGradient(p1.x, p0.y)];
  vec3 d11 = directions[pickGradient(p1.x, p1.y)];

  float nx = (pos.x - float(p0.x)) / float(u_cellWidth);
  float ny = (pos.y - float(p0.y)) / float(u_cellWidth);

  // corner dot products
  float n00 = dot(d00.xy, vec2(nx, ny));
  float n10 = dot(d10.xy, vec2(nx - 1.0, ny));
  float n01 = dot(d01.xy, vec2(nx, ny - 1.0));
  float n11 = dot(d11.xy, vec2(nx - 1.0, ny - 1.0));

  // smoothstep weights and their derivatives
  // cubic function 3t^2 - 2t^3
  float wu = 3.0 * nx * nx - 2.0 * nx * nx * nx;
  float wv = 3.0 * ny * ny - 2.0 * ny * ny * ny;

  float su = 6.0 * nx * (1.0 - nx); // d(wu)/d(nx)
  float sv = 6.0 * ny * (1.0 - ny); // d(wv)/d(ny)

  // bilinear rows
  float A = n00 * (1.0 - wu) + n10 * wu;
  float B = n01 * (1.0 - wu) + n11 * wu;
  float value = A * (1.0 - wv) + B * wv;

  // analytical gradient in cell-normalized space
  float dA_dnx = d00.x * (1.0 - wu) + d10.x * wu + (n10 - n00) * su;
  float dB_dnx = d01.x * (1.0 - wu) + d11.x * wu + (n11 - n01) * su;

  float dA_dny = d00.y * (1.0 - wu) + d10.y * wu;
  float dB_dny = d01.y * (1.0 - wu) + d11.y * wu;

  // convert from cell-normalized space to world space
  float invCW = 1.0 / float(u_cellWidth);
  float df_dx = dA_dnx * (1.0 - wv) + dB_dnx * wv;
  float df_dy = dA_dny * (1.0 - wv) + dB_dny * wv + (B - A) * sv;

  return vec3(value, df_dx, df_dy);
}
