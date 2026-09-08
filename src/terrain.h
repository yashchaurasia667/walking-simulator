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
  glm::ivec2 _last_player_chunk = glm::ivec2(INT_MAX);

  int _chunk_width, _cell_width, _rez = 20, _draw_dist = 2;
  int _tess_min_level = 4, _tess_max_level = 64;
  float _tess_min_dist = 20, _tess_max_dist = 2000;
  float _tex_scale = 15.5f, _slope_strength = 1.2f, _snow_slope_max = 0.3f,
        _snow_slope_min = 1.2f;

  // noise vars
  int _noise_seed = 5;
  int _noise_pass = 10;
  float _amp = 128.0f, _freq = 0.29f, _persistance = 0.43f, _lacunarity = 2.7f;

  // lighting/color vars
  glm::vec3 _light_dirn = glm::vec3(-0.35f, 0.3f, 1.0f),
            _light_color = glm::vec3(0.47f, 0.34f, 0.26f),
            _ambient = glm::vec3(0.62f, 0.59f, 1.0f),
            _terrain_color = glm::vec3(0.35f, 0.28f, 0.15f),
            _snow_color = glm::vec3(1.0f),
            _water_color = glm::vec3(0.2f, 0.5f, 0.7f);

  // shaders
  ComputeShader _noise_shader;
  Shader _shader;

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
  int _rez_scale = 1;
  std::vector<float> _vertices;
  std::vector<Chunk> _chunks;
  std::queue<int> _regenQueue;
  VertexArray _vao;
  VertexBuffer _vbo;
  VertexBufferLayout _layout;

  void generateChunkTextures();
  void generateVertices();
  void uploadVertexData();
  void generateChunks();
  void generateChunkHeightmap(int chunkIdx);
  void updateChunks(glm::vec3 playerPos);
  void processRegenQueue();
};
