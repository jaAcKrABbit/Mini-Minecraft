#include "terrain.h"
#include "cube.h"
#include <stdexcept>
#include <iostream>
#include <math.h>
#include <random>

enum BiomeType { Grass,Mountain };

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), m_geomCube(context), mp_context(context)
{}

Terrain::~Terrain() {
    m_geomCube.destroyVBOdata();
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}

//map to their nearest Zone corner
bool Terrain::hasZoneAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 64.f));
    int zFloor = static_cast<int>(glm::floor(z / 64.f));
    return m_generatedTerrain.find(toKey(64 * xFloor, 64 * zFloor)) != m_generatedTerrain.end();
}

uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!" + "  Chunk xz: " + std::to_string(x) + "   " + std::to_string(z));
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(ShaderProgram *shaderProgram) {
    for(auto& chunk: m_chunks) {
        auto &c = chunk.second;
        if(c->hasVBOdata) {
            //chunk first is key, chunk second is the chunk uPtr
            int x = toCoords(chunk.first).x;
            int z = toCoords(chunk.first).y;
            shaderProgram->setModelMatrix(glm::translate(glm::mat4(), glm::vec3(x, 0, z)));
            shaderProgram->drawOpaque(*c);
        }
    }
}

void Terrain::drawTransparent(ShaderProgram* shaderProgram) {
    for(auto& chunk: m_chunks) {
        auto &c = chunk.second;
        if(c->hasVBOdata) {
            //chunk first is key, chunk second is the chunk uPtr
            int x = toCoords(chunk.first).x;
            int z = toCoords(chunk.first).y;
            shaderProgram->setModelMatrix(glm::translate(glm::mat4(), glm::vec3(x, 0, z)));
            shaderProgram->drawTransparent(*c);
        }
    }
}

void Terrain::initializeSnow()
{
    m_geomCube.clearOffsetBuf();
    m_geomCube.clearColorBuf();

    std::vector<glm::vec3> offsets, colors;
    int minX = 0;
    int maxX = 64;
    int minZ = 0;
    int maxZ = 64;


    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            for(int i = 0; i < 16; ++i) {
                for(int j = 0; j < 256; ++j) {
                    for(int k = 0; k < 16; ++k) {
                        float prob = (rand() % 100)  / 100.f;
                        if (prob > 0.95f) {
                            float local_offset = (rand() % 1000)  / 1000.f;
                            float choice = (rand() % 2)  / 2.f;
                            if (choice > 0.5f) {
                                offsets.push_back(glm::vec3(i+x + local_offset, j + local_offset, k+z+local_offset));
                            } else {
                                offsets.push_back(glm::vec3(i+x - local_offset, j - local_offset, k+z-local_offset));
                            }

                            colors.push_back(glm::vec3(1.f));
                        }
                    }
                }
            }
        }
    }

    m_geomCube.createInstancedVBOdata(offsets, colors);
    m_geomCube.offsets = offsets;
    m_geomCube.colors = colors;
}

void Terrain::drawSnow(ShaderProgram* shaderproram) {
    m_geomCube.createInstancedVBOdata(m_geomCube.offsets, m_geomCube.colors);
    shaderproram->drawInstanced(m_geomCube);
}


void Terrain::CreateTestScene()
{
    // TODO: DELETE THIS LINE WHEN YOU DELETE m_geomCube!
    m_geomCube.createVBOdata();
}

void Terrain::updateTerrain(glm::vec3 playerPos, glm::vec3 prevPos) {
    tryExpansion(playerPos, prevPos);
    checkThreadResults();
}

void Terrain::tryExpansion(glm::vec3 playerPos, glm::vec3 prevPos) {
    //  glm::ivec2 currChunkPos(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 currZone = glm::ivec2(glm::floor(playerPos.x / 64.f) * 64.f, glm::floor(playerPos.z / 64.f) * 64.f);
    glm::ivec2 prevZone = glm::ivec2(glm::floor(prevPos.x / 64.f) * 64.f, glm::floor(prevPos.z / 64.f) * 64.f);



    //5x5
    QSet<int64_t> currZoneNeighrbors = terrainZonesBoarderingZone(currZone);
    QSet<int64_t> prevZoneNeighrbors = terrainZonesBoarderingZone(prevZone);


    //destroy chunks that are too far away
    for(auto id: prevZoneNeighrbors) {
        if(!currZoneNeighrbors.contains(id)) {
            glm::ivec2 coord = toCoords(id);
            //delete all chunks in that zone
            for(int x = coord.x; x < coord.x + 64; x += 16) {
                for(int z = coord.y; z < coord.y + 64; z += 16) {
                    auto& chunk = getChunkAt(x, z);
                    chunk->destroyVBOdata();
                }
            }
        }
    }
    for(auto id: currZoneNeighrbors) {
        glm::ivec2 zone = toCoords(id);
        //if already set up chunks, feed them vbo data
        if(hasZoneAt(zone.x,zone.y)) {
            if(!prevZoneNeighrbors.contains(id)) {
                for(int x = zone.x; x < zone.x + 64; x += 16) {
                    for(int z = zone.y; z < zone.y + 64; z += 16) {
                        auto& chunk = getChunkAt(x, z);
                        //TODO: createVBOWorker to createVBOData for chunks
                        spawnVBOWorker(chunk.get());
                    }
                }
            }
        }else{
            //TODO: createFBMWorker to setup blocks for a zone
            spawnFBMWorker(id);
        }
    }
    if(currZone == prevZone) return;

}

QSet<int64_t> Terrain::terrainZonesBoarderingZone(glm::ivec2 zone) {
    QSet<int64_t> neighbors;
    //5x5
    for (int i = -128; i <= 128; i += 64) {
        for (int j = -128; j <= 128; j += 64) {
            neighbors.insert(toKey(zone.x + i, zone.y + j));
        }
    }
    return neighbors;
}

void Terrain::spawnVBOWorker(Chunk *chunk) {
    //TODO: Create VBO data for each chunk
    VBOWorker* worker = new VBOWorker(chunk, &m_VBOData, &m_chunksThatHaveVBOsLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::spawnVBOWorkers(const std::vector<Chunk*> chunksNeedingVBOData) {
    for(auto chunk: chunksNeedingVBOData) {
        spawnVBOWorker(chunk);
    }
}

void Terrain::spawnFBMWorker(int64_t id) {
    m_generatedTerrain.insert(id);
    std::vector<Chunk*> chunksforWorker;
    glm::ivec2 zone(toCoords(id));
    //setup 16 chunks on generated terrain zone
    for(int x = zone.x; x < zone.x + 64; x += 16) {
        for(int z = zone.y; z < zone.y + 64; z += 16) {
            Chunk *c = instantiateChunkAt(x,z);
            c->m_position = glm::ivec2(x,z);
            c->m_count = 0;
            chunksforWorker.push_back(c);
        }
    }
    FBMWorker *worker = new FBMWorker(zone, chunksforWorker, &m_chunksThatHaveBlockTypeData, &m_chunksThatHaveBlockTypeDataLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::checkThreadResults() {
    //From slides
    //TODO: Handle worker results on the main thread and send vbo data to GPU
    m_chunksThatHaveBlockTypeDataLock.lock();
    spawnVBOWorkers(m_chunksThatHaveBlockTypeData);
    m_chunksThatHaveBlockTypeData.clear();
    m_chunksThatHaveBlockTypeDataLock.unlock();

    m_chunksThatHaveVBOsLock.lock();
    for(auto& cd: m_VBOData) {
        cd.mp_chunk->createVBO(cd.m_trans, cd.m_transIdx, cd.m_op, cd.m_opIdx);
        cd.mp_chunk->hasVBOdata = true;
    }
    m_VBOData.clear();
    m_chunksThatHaveVBOsLock.unlock();

}

void Terrain::CreateSnow() {
    m_geomCube.createVBOdata();
}


void Terrain::createTestTerrainForChunk(glm::ivec2 newChunkPos) {
    //setup test chunk
    for(int x = 0; x < 16; ++x) {
        for(int z = 0; z < 16; ++z) {
            if((x + z) % 2 == 0) {
                setBlockAt(newChunkPos.x + x, 128, newChunkPos.y + z, STONE);
            }
            else {
                setBlockAt(newChunkPos.x + x, 128,newChunkPos.y + z, DIRT);
            }
        }
    }
}

