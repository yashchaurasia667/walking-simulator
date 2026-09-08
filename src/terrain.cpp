#include "terrain.h"
#include <glm/ext/matrix_transform.hpp>

Terrain::Terrain(int chunkWidth, int cellWidth, int noiseSeed, unsigned int rez,
                 int drawDist) {
  this->chunkWidth = chunkWidth;
  this->cellWidth = cellWidth;
  this->noiseSeed = noiseSeed;
  this->rez = rez;
  this->drawDist = drawDist;

  layout.push<float>(3);
  layout.push<float>(2);
  generateVertices();
  uploadVertexData();
  generateChunks();
}

Terrain::~Terrain() {
  for (Chunk &c : chunks) {
    if (c.heightMap != 0)
      glDeleteTextures(1, &c.heightMap);
  }
}

void Terrain::generateChunks() {
  chunks.clear();

  // drawDist -> 1 to n
  unsigned int n = drawDist * 2 - 1;
  int half = (int)(n / 2);
  for (int cy = -half; cy <= half; cy++) {
    for (int cx = -half; cx <= half; cx++) {
      Chunk c;
      c.coord = glm::ivec2(cx, cy);
      c.ready = false;
      c.needsRegen = false;
      chunks.push_back(c);
    }
  }
}

void Terrain::generateChunkTextures() {
  for (unsigned int i = 0; i < chunks.size(); i++) {
    glGenTextures(1, &chunks[i].heightMap);
    glBindTexture(GL_TEXTURE_2D, chunks[i].heightMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, chunkWidth, chunkWidth, 0,
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
  noiseShader = ComputeShader(compute);
  shader = Shader(vert, frag, geometry, tess_control, tess_evaluation);

  initTerrain();
}

void Terrain::initTerrain() {
  generateChunkTextures();
  for (int i = 0; i < (int)chunks.size(); i++) {
    generateChunkHeightmap(i);
  }
}

void Terrain::generateChunkHeightmap(int idx) {
  Chunk &c = chunks[idx];
  // world offset: chunk coord × chunk size in world units
  glm::vec2 worldOffset = glm::vec2(c.coord) * (float)chunkWidth;

  noiseShader.bind();
  noiseShader.setInt("u_seed", noiseSeed);
  noiseShader.setInt("u_cellWidth", cellWidth);
  noiseShader.setInt("u_chunkWidth", chunkWidth);
  noiseShader.setInt("u_noisePass", noisePass);
  noiseShader.setFloat("u_amplitude", amp);
  noiseShader.setFloat("u_frequency", freq);
  noiseShader.setFloat("u_slopeStrength", slopeStrength);
  noiseShader.setFloat("u_lacunarity", lacunarity);
  noiseShader.setFloat("u_persistance", persistance);
  noiseShader.setInt("u_heightMap", 0);
  noiseShader.setVec2("u_chunkOffset", worldOffset);

  glBindImageTexture(0, c.heightMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
  glDispatchCompute((chunkWidth + 15) / 16, (chunkWidth + 15) / 16, 1);
  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

  c.ready = true;
  c.needsRegen = false;
}

void Terrain::updateChunks(glm::vec3 playerPos) {
  glm::ivec2 playerChunk =
      glm::ivec2((int)std::round(playerPos.x / chunkWidth),
                 (int)std::round(playerPos.z / chunkWidth));

  if (playerChunk == lastPlayerChunk)
    return;
  lastPlayerChunk = playerChunk;

  unsigned int n = drawDist * 2 - 1;
  int half = (int)(n / 2);

  for (int i = 0; i < (int)chunks.size(); i++) {
    glm::ivec2 diff = chunks[i].coord - playerChunk;
    bool outOfRange = std::abs(diff.x) > half || std::abs(diff.y) > half;
    if (!outOfRange)
      continue;

    for (int cy = -half; cy <= half; cy++) {
      for (int cx = -half; cx <= half; cx++) {
        glm::ivec2 needed = playerChunk + glm::ivec2(cx, cy);
        bool covered = false;
        for (Chunk &c : chunks) {
          if (c.coord == needed) {
            covered = true;
            break;
          }
        }
        if (!covered) {
          // recycle this chunk to the new coord
          chunks[i].coord = needed;
          chunks[i].ready = false;
          chunks[i].needsRegen = true;
          regenQueue.push(i);
          goto nextChunk; // break both loops, move to next chunk index
        }
      }
    }
  nextChunk:;
  }
}

void Terrain::processRegenQueue() {
  if (regenQueue.empty())
    return;

  int idx = regenQueue.front();
  regenQueue.pop();
  generateChunkHeightmap(idx);
}

void Terrain::uploadVertexData() {
  vao.bind();
  vbo.bind();
  vbo.setData(vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
  vao.addBuffer(vbo, layout);
}

void Terrain::generateVertices() {
  vertices.clear();
  for (int i = 0; i < rez; i++) {
    for (int j = 0; j < rez; j++) {
      vertices.push_back(-chunkWidth / 2.0f + (chunkWidth * i) / (float)rez);
      vertices.push_back(0.0f);
      vertices.push_back(-chunkWidth / 2.0f + (chunkWidth * j) / (float)rez);
      vertices.push_back(i / (float)rez);
      vertices.push_back(j / (float)rez);

      vertices.push_back(-chunkWidth / 2.0f +
                         (chunkWidth * (i + 1)) / (float)rez);
      vertices.push_back(0.0f);
      vertices.push_back(-chunkWidth / 2.0f + (chunkWidth * j) / (float)rez);
      vertices.push_back((i + 1) / (float)rez);
      vertices.push_back(j / (float)rez);

      vertices.push_back(-chunkWidth / 2.0f + (chunkWidth * i) / (float)rez);
      vertices.push_back(0.0f);
      vertices.push_back(-chunkWidth / 2.0f +
                         (chunkWidth * (j + 1)) / (float)rez);
      vertices.push_back(i / (float)rez);
      vertices.push_back((j + 1) / (float)rez);

      vertices.push_back(-chunkWidth / 2.0f +
                         (chunkWidth * (i + 1)) / (float)rez);
      vertices.push_back(0.0f);
      vertices.push_back(-chunkWidth / 2.0f +
                         (chunkWidth * (j + 1)) / (float)rez);
      vertices.push_back((i + 1) / (float)rez);
      vertices.push_back((j + 1) / (float)rez);
    }
  }
}

void Terrain::render(Camera camera, glm::mat4 model, glm::mat4 projection) {
  updateChunks(camera.getPos());
  processRegenQueue();

  shader.bind();
  for (Chunk &c : chunks) {
    if (!c.ready)
      continue;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, c.heightMap);

    glm::vec2 worldPos = glm::vec2(c.coord) * (float)chunkWidth;
    glm::mat4 chunkModel = glm::translate(
        glm::mat4(1.0f), glm::vec3(worldPos.x, 0.0f, worldPos.y));

    shader.setMat4("model", chunkModel);
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", projection);

    shader.setInt("heightMap", 0);
    shader.setInt("MIN_TESS_LEVEL", tess_min_level);
    shader.setInt("MAX_TESS_LEVEL", tess_max_level);
    shader.setFloat("MIN_DISTANCE", tess_min_dist);
    shader.setFloat("MAX_DISTANCE", tess_max_dist);

    shader.setFloat("u_amplitude", amp);
    shader.setFloat("u_chunkWidth", (float)chunkWidth);
    shader.setInt("u_noisePass", noisePass);

    shader.setVec3("u_lightDir", glm::normalize(lightDir));
    shader.setVec3("u_lightColor", lightColor);
    shader.setVec3("u_ambientColor", ambient);
    shader.setVec3("u_viewPos", camera.getPos());

    shader.setFloat("u_texScale", texScale);
    shader.setInt("u_normalMap", 1);
    shader.setVec3("u_terrainColor", terrainColor);
    shader.setVec3("u_waterColor", waterColor);
    shader.setVec3("u_snowColor", snowColor);
    shader.setFloat("u_snowSlopeMax", snowSlopeMax);
    shader.setFloat("u_snowSlopeMin", snowSlopeMin);

    vao.bind();
    glDrawArrays(GL_PATCHES, 0, rez * rez * 4);
  }
}

void Terrain::reinit() {
  generateChunks();
  initTerrain();
  generateVertices();
  uploadVertexData();
}
