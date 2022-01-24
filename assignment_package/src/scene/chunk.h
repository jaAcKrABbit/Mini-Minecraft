#pragma once
#include "drawable.h"
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <iostream>

class Chunk;
//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, LAVA, BEDROCK, SAND, SNOW
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

struct Neighbor {
    Direction direction;
    glm::ivec3 vecDirection;
    std::array<glm::ivec3, 4> vertPos;
};

struct ChunkVBOData {
    Chunk* mp_chunk;
    //without opaue and transparent yet
    std::vector<glm::vec4> m_trans;
    std::vector<glm::vec4> m_op;

    std::vector<GLuint> m_transIdx;
    std::vector<GLuint> m_opIdx;

    ChunkVBOData(Chunk* c): mp_chunk(c), m_trans{}, m_op{}, m_transIdx{}, m_opIdx{}
    {}

};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable{
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;
    std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec4, EnumHash>, EnumHash> m_uvs {
        {WATER, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                                    {XNEG, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                                    {YPOS, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                                    {YNEG, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                                    {ZPOS, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                                    {ZNEG, glm::vec4(15.f/16.f, 3.f/16.f, 0, 0)}}},
        {LAVA, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(14.f/16.f, 1.f/16.f, 0, 0)},
                                                                   {XNEG, glm::vec4(14.f/16.f, 1.f/16.f, 0, 0)},
                                                                   {YPOS, glm::vec4(14.f/16.f, 1.f/16.f, 0, 0)},
                                                                   {YNEG, glm::vec4(14.f/16.f, 1.f/16.f, 0, 0)},
                                                                   {ZPOS, glm::vec4(14.f/16.f, 1.f/16.f, 0, 0)},
                                                                   {ZNEG, glm::vec4(14.f/16.f, 1.f/16.f, 0, 0)}}},
        {GRASS, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {XNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {YPOS, glm::vec4(8.f/16.f, 13.f/16.f, 0, 0)},
                                                                    {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {ZPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {ZNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)}}},
        {DIRT, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                   {XNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                   {YPOS, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                   {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                   {ZPOS, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                   {ZNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)}}},
        {STONE, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {XNEG, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {YPOS, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {YNEG, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {ZPOS, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                                    {ZNEG, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)}}},
        {SNOW, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                                    {XNEG, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                                    {YPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                                    {YNEG, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                                    {ZPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                                    {ZNEG, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)}}},
        {SAND, std::unordered_map<Direction, glm::vec4, EnumHash> {{XPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                                    {XNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                                    {YPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                                    {YNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                                    {ZPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                                    {ZNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)}}}

    };

public:
    Chunk(OpenGLContext* context);
    void createVBOdata() override;
    void destroyVBOdata() override;
    void createVBO(std::vector<glm::vec4> &interleaved_trans, std::vector<GLuint> &idx_trans, std::vector<glm::vec4> &interleaved_opq, std::vector<GLuint> &idx_opq);
    GLenum drawMode() override;
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);
    BlockType getNeighborBlock(int x, int y, int z);
    void generateTestTerrain(glm::ivec2 chunkPos);
    glm::ivec2 m_position; // temp
    ChunkVBOData m_chunkVBOData;

    bool hasVBOdata;

    friend class Terrain;

    void GenerateChunkAt(glm::vec2 xz);

};
