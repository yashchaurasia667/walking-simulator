#include "terrain.h"
#include <glm/ext/matrix_transform.hpp>

Terrain::Terrain(int chunkWidth, int cellWidth, int noiseSeed, unsigned int rez,
                 int drawDist) {
  this->_chunk_width = chunkWidth;
  this->_cell_width = cellWidth;
  this->_noise_seed = noiseSeed;
  this->_rez = rez;
  this->_draw_dist = drawDist;

  _layout.push<float>(3);
  _layout.push<float>(2);
  generateVertices();
  uploadVertexData();
  generateChunks();
}

Terrain::~Terrain() {
  for (Chunk &c : _chunks) {
    if (c.heightMap != 0)
      glDeleteTextures(1, &c.heightMap);
  }
}

void Terrain::generateChunks() {
  _chunks.clear();

  // drawDist -> 1 to n
  unsigned int n = _draw_dist * 2 - 1;
  int half = (int)(n / 2);
  for (int cy = -half; cy <= half; cy++) {
    for (int cx = -half; cx <= half; cx++) {
      Chunk c;
      c.coord = glm::ivec2(cx, cy);
      c.ready = false;
      c.needsRegen = false;
      _chunks.push_back(c);
    }
  }
}

void Terrain::generateChunkTextures() {
  for (unsigned int i = 0; i < _chunks.size(); i++) {
    glGenTextures(1, &_chunks[i].heightMap);
    glBindTexture(GL_TEXTURE_2D, _chunks[i].heightMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _chunk_width, _chunk_width, 0,
                 GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
}

void Terrain::initShader(const char *compute, const char *vert,
                         const char *frag, const char *geometry,
                         const char *tess_control,
                         const char *tess_evaluation) {
  _noise_shader = ComputeShader(compute);
  _shader = Shader(vert, frag, geometry, tess_control, tess_evaluation);

  initTerrain();
}

void Terrain::initTerrain() {
  generateChunkTextures();
  for (int i = 0; i < (int)_chunks.size(); i++) {
    generateChunkHeightmap(i);
  }
}

void Terrain::generateChunkHeightmap(int idx) {
  Chunk &c = _chunks[idx];
  // world offset: chunk coord × chunk size in world units
  glm::vec2 worldOffset = glm::vec2(c.coord) * (float)_chunk_width;

  _noise_shader.bind();
  _noise_shader.setInt("u_seed", _noise_seed);
  _noise_shader.setInt("u_cellWidth", _cell_width);
  _noise_shader.setInt("u_chunkWidth", _chunk_width);
  _noise_shader.setInt("u_noisePass", _noise_pass);
  _noise_shader.setFloat("u_amplitude", _amp);
  _noise_shader.setFloat("u_frequency", _freq);
  _noise_shader.setFloat("u_slopeStrength", _slope_strength);
  _noise_shader.setFloat("u_lacunarity", _lacunarity);
  _noise_shader.setFloat("u_persistance", _persistance);
  _noise_shader.setInt("u_heightMap", 0);
  _noise_shader.setVec2("u_chunkOffset", worldOffset);

  glBindImageTexture(0, c.heightMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
  glDispatchCompute((_chunk_width + 15) / 16, (_chunk_width + 15) / 16, 1);
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

  c.ready = true;
  c.needsRegen = false;
}

void Terrain::updateChunks(glm::vec3 playerPos) {
  glm::ivec2 playerChunk =
      glm::ivec2((int)std::round(playerPos.x / _chunk_width),
                 (int)std::round(playerPos.z / _chunk_width));

  if (playerChunk == _last_player_chunk)
    return;
  _last_player_chunk = playerChunk;

  unsigned int n = _draw_dist * 2 - 1;
  int half = (int)(n / 2);

  for (int i = 0; i < (int)_chunks.size(); i++) {
    glm::ivec2 diff = _chunks[i].coord - playerChunk;
    bool outOfRange = std::abs(diff.x) > half || std::abs(diff.y) > half;
    if (!outOfRange)
      continue;

    for (int cy = -half; cy <= half; cy++) {
      for (int cx = -half; cx <= half; cx++) {
        glm::ivec2 needed = playerChunk + glm::ivec2(cx, cy);
        bool covered = false;
        for (Chunk &c : _chunks) {
          if (c.coord == needed) {
            covered = true;
            break;
          }
        }
        if (!covered) {
          // recycle this chunk to the new coord
          _chunks[i].coord = needed;
          _chunks[i].ready = false;
          _chunks[i].needsRegen = true;
          _regenQueue.push(i);
          goto nextChunk; // break both loops, move to next chunk index
        }
      }
    }
  nextChunk:;
  }
}

void Terrain::processRegenQueue() {
  if (_regenQueue.empty())
    return;

  int idx = _regenQueue.front();
  _regenQueue.pop();
  generateChunkHeightmap(idx);
}

void Terrain::uploadVertexData() {
  _vao.bind();
  _vbo.bind();
  _vbo.setData(_vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);
  _vao.addBuffer(_vbo, _layout);
}

void Terrain::generateVertices() {
  _vertices.clear();
  for (int i = 0; i < _rez; i++) {
    for (int j = 0; j < _rez; j++) {
      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * i) / (float)_rez);
      _vertices.push_back(0.0f);
      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * j) / (float)_rez);
      _vertices.push_back(i / (float)_rez);
      _vertices.push_back(j / (float)_rez);

      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * (i + 1)) / (float)_rez);
      _vertices.push_back(0.0f);
      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * j) / (float)_rez);
      _vertices.push_back((i + 1) / (float)_rez);
      _vertices.push_back(j / (float)_rez);

      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * i) / (float)_rez);
      _vertices.push_back(0.0f);
      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * (j + 1)) / (float)_rez);
      _vertices.push_back(i / (float)_rez);
      _vertices.push_back((j + 1) / (float)_rez);

      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * (i + 1)) / (float)_rez);
      _vertices.push_back(0.0f);
      _vertices.push_back(-_chunk_width / 2.0f +
                          (_chunk_width * (j + 1)) / (float)_rez);
      _vertices.push_back((i + 1) / (float)_rez);
      _vertices.push_back((j + 1) / (float)_rez);
    }
  }
}

void Terrain::render(Camera camera, glm::mat4 model, glm::mat4 projection) {
  updateChunks(camera.getPos());
  processRegenQueue();

  _shader.bind();
  for (Chunk &c : _chunks) {
    if (!c.ready)
      continue;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, c.heightMap);

    glm::vec2 worldPos = glm::vec2(c.coord) * (float)_chunk_width;
    glm::mat4 chunkModel = glm::translate(
        glm::mat4(1.0f), glm::vec3(worldPos.x, 0.0f, worldPos.y));

    _shader.setMat4("model", chunkModel);
    _shader.setMat4("view", camera.getViewMatrix());
    _shader.setMat4("projection", projection);

    _shader.setInt("heightMap", 0);
    _shader.setInt("MIN_TESS_LEVEL", _tess_min_level);
    _shader.setInt("MAX_TESS_LEVEL", _tess_max_level);
    _shader.setFloat("MIN_DISTANCE", _tess_min_dist);
    _shader.setFloat("MAX_DISTANCE", _tess_max_dist);

    _shader.setFloat("u_amplitude", _amp);
    _shader.setFloat("u_chunkWidth", (float)_chunk_width);
    _shader.setInt("u_noisePass", _noise_pass);

    _shader.setVec3("u_lightDir", glm::normalize(_light_dirn));
    _shader.setVec3("u_lightColor", _light_color);
    _shader.setVec3("u_ambientColor", _ambient);
    _shader.setVec3("u_viewPos", camera.getPos());

    _shader.setFloat("u_texScale", _tex_scale);
    _shader.setInt("u_normalMap", 1);
    _shader.setVec3("u_terrainColor", _terrain_color);
    _shader.setVec3("u_waterColor", _water_color);
    _shader.setVec3("u_snowColor", _snow_color);
    _shader.setFloat("u_snowSlopeMax", _snow_slope_max);
    _shader.setFloat("u_snowSlopeMin", _snow_slope_min);

    _vao.bind();
    glDrawArrays(GL_PATCHES, 0, _rez * _rez * 4);
  }
}

void Terrain::reinit() {
  generateChunks();
  initTerrain();
  generateVertices();
  uploadVertexData();
}
