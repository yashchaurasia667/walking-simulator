#pragma once
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <camera.h>
#include <shader.h>
#include <vertexArray.h>
#include <vertexBuffer.h>
#include <vertexBufferLayout.h>

#include <climits>
#include <queue>
#include <vector>

struct Chunk {
  glm::ivec2 coord;
  unsigned int heightMap = 0;
  bool ready = false;
  bool needsRegen = false;
};

class Terrain {
public:
  // forces update on first frame
  glm::ivec2 lastPlayerChunk = glm::ivec2(INT_MAX);

  int chunkWidth, cellWidth, rez = 20, drawDist = 2;
  int tess_min_level = 4, tess_max_level = 64;
  float tess_min_dist = 20, tess_max_dist = 2000;
  float texScale = 15.5f, slopeStrength = 1.2f, snowSlopeMax = 0.3f,
        snowSlopeMin = 1.2f;

  // noise vars
  int noiseSeed = 5;
  int noisePass = 10;
  float amp = 128.0f, freq = 0.29f, persistance = 0.43f, lacunarity = 2.7f;

  // lighting/color vars
  glm::vec3 lightDir = glm::vec3(-0.35f, 0.3f, 1.0f),
            lightColor = glm::vec3(0.47f, 0.34f, 0.26f),
            ambient = glm::vec3(0.62f, 0.59f, 1.0f),
            terrainColor = glm::vec3(0.35f, 0.28f, 0.15f),
            snowColor = glm::vec3(1.0f),
            waterColor = glm::vec3(0.2f, 0.5f, 0.7f);

  // shaders
  ComputeShader noiseShader;
  Shader shader;

  Terrain(int chunkWidth = 1000, int cellWidth = 200, int noiseSeed = 0,
          unsigned int rez = 20, int drawDist = 3);
  ~Terrain();

  void initShader(const char *compute, const char *vert, const char *frag,
                  const char *geometry = nullptr,
                  const char *tess_control = nullptr,
                  const char *tess_evaluation = nullptr);

  void initTerrain();
  void render(Camera camera, glm::mat4 model, glm::mat4 projection);
  void reinit();

private:
  int rezScale = 1;
  std::vector<float> vertices;
  std::vector<Chunk> chunks;
  std::queue<int> regenQueue;
  VertexArray vao;
  VertexBuffer vbo;
  VertexBufferLayout layout;

  void generateChunkTextures();
  void generateVertices();
  void uploadVertexData();
  void generateChunks();
  void generateChunkHeightmap(int chunkIdx);
  void updateChunks(glm::vec3 playerPos);
  void processRegenQueue();
};
