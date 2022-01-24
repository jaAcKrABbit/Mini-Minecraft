#include "chunk.h"
float WorleyNoise(glm::vec2 uv);
float surflet3D(glm::vec3 p, glm::vec3 gridPoint);
float perlinNoise3D(glm::vec3 p);
float LERP(float x,float y, float fract);
float interpNoise2D(float x, float y);
float fbm(float x, float y);
float mountain(glm::vec2 pos);
float interpolate(glm::vec2 uv);
float grass(glm::vec2 pos);
float Moisture(float worldx,float worldz);

enum BiomeType { Grass,Mountain,Snowland,Desert };

Chunk::Chunk(OpenGLContext* context) : Drawable(context), m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
    m_position(glm::ivec2(0,0)), m_chunkVBOData(this), hasVBOdata(false)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};


void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

Neighbor top = {YPOS, glm::ivec3(0, 1, 0), {glm::ivec3(0, 1, 0),glm::ivec3(1, 1, 0), glm::ivec3(1, 1, 1), glm::ivec3(0, 1, 1)}};
//Neighbor bot = {YNEG, glm::ivec3(0, -1, 0), {glm::ivec3(0, 0, 1),glm::ivec3(1, 0, 1), glm::ivec3(1, 0, 0), glm::ivec3(0, 0, 0)}};
Neighbor bot = {YNEG, glm::ivec3(0, -1, 0), {glm::ivec3(0, 0, 0),glm::ivec3(1, 0, 0), glm::ivec3(1, 0, 1), glm::ivec3(0, 0, 1)}};

Neighbor left = {XNEG, glm::ivec3(-1, 0, 0), {glm::ivec3(0, 0, 1), glm::ivec3(0, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, 1, 1)}};
//Neighbor right = {XPOS, glm::ivec3(1, 0, 0), {glm::ivec3(1, 1, 0), glm::ivec3(1, 1, 1), glm::ivec3(1, 0, 1), glm::ivec3(1, 0, 0)}};
Neighbor right = {XPOS, glm::ivec3(1, 0, 0), {glm::ivec3(1, 0, 0), glm::ivec3(1, 0, 1), glm::ivec3(1, 1, 1), glm::ivec3(1, 1, 0)}};

Neighbor front = {ZNEG, glm::ivec3(0, 0, -1), {glm::ivec3(0, 0, 0), glm::ivec3(1, 0, 0), glm::ivec3(1, 1, 0), glm::ivec3(0, 1, 0)}};
//Neighbor back = {ZPOS, glm::ivec3(0, 0, 1), {glm::ivec3(0, 1, 1),glm::ivec3(1, 1, 1), glm::ivec3(1, 0, 1), glm::ivec3(0, 0, 1)}};

Neighbor back = {ZPOS, glm::ivec3(0, 0, 1), {glm::ivec3(0, 0, 1),glm::ivec3(1, 0, 1), glm::ivec3(1, 1, 1), glm::ivec3(0, 1, 1)}};

const static std::array<Neighbor, 6> neighbors = {right, left, top, bot, back, front};

const static std::unordered_map<int, glm::vec4> mv_vertex = {{0, glm::vec4(0, 0, 0, 0)}, {1, glm::vec4(1.f / 16.f, 0, 0, 0)}, {2, glm::vec4(1.f / 16.f, 1.f / 16.f, 0, 0)}, {3, glm::vec4(0, 1.f / 16.f, 0, 0)}};


void Chunk::destroyVBOdata() {
    Drawable::destroyVBOdata();
    this->hasVBOdata = false;
}

void Chunk::createVBOdata() {

    // create opaque data
    std::vector<glm::vec4> interleavedData_opaque;
    std::vector<GLuint> idx_opaque;
    int m_quatOpaqueCount = 0;

    // create transparent data
    std::vector<glm::vec4> interleavedData_transparent;
    std::vector<GLuint> idx_transparent;
    int m_quatTransparentCount = 0;

    for(int x = 0; x < 16; x++) {
        for(int z = 0; z < 16; z++) {
            for(int y = 0; y < 256; y++) {
                BlockType t = getBlockAt(x, y, z);
                if (t != EMPTY) {
                    for (auto & neigh : neighbors) {
                        glm::ivec3 neighborPos = glm::ivec3(x + neigh.vecDirection.x, y + neigh.vecDirection.y, z + neigh.vecDirection.z);
                        BlockType neighborType = getNeighborBlock(neighborPos.x, neighborPos.y, neighborPos.z);
                        if (neighborType == EMPTY) {
                            Direction direction = neigh.direction;
                            glm::vec4 uv = m_uvs.at(t).at(direction);
                            for (int i = 0; i < 4; i++) {
                                glm::vec4 vertexUV = uv + mv_vertex.at(i);
                                glm::vec4 position = glm::vec4(neigh.vertPos[i].x + x, neigh.vertPos[i].y + y, neigh.vertPos[i].z + z, 1);
                                glm::vec4 normal = glm::vec4(neigh.vecDirection, 1);
                              // std::cout << "break" << std::endl;
                                switch(t) {
                                case GRASS:
                                    interleavedData_opaque.push_back(position);
                                    interleavedData_opaque.push_back(normal);
                                    vertexUV.z = 1;
                                    vertexUV.w = 1;
                                    interleavedData_opaque.push_back(vertexUV);
                                    break;
                                case DIRT:
                                    interleavedData_opaque.push_back(position);
                                    interleavedData_opaque.push_back(normal);
                                    vertexUV.z = 1;
                                    vertexUV.w = 1;
                                    interleavedData_opaque.push_back(vertexUV);
                                    break;
                                case STONE:
                                    interleavedData_opaque.push_back(position);
                                    interleavedData_opaque.push_back(normal);
                                    vertexUV.z = 1;
                                    vertexUV.w = 1;
                                    interleavedData_opaque.push_back(vertexUV);
                                    break;
                                case WATER:
                                    if (neighborType == EMPTY) {
                                        interleavedData_transparent.push_back(position);
                                        interleavedData_transparent.push_back(normal);
                                        vertexUV.z = 0;
                                        vertexUV.w = 0.8;
                                        interleavedData_transparent.push_back(vertexUV);
                                    } else {
                                        continue;
                                    }
                                    break;
                                case LAVA:
                                    interleavedData_opaque.push_back(position);
                                    interleavedData_opaque.push_back(normal);
                                    vertexUV.z = 0;
                                    vertexUV.w = 1;
                                    interleavedData_opaque.push_back(vertexUV);
                                    break;
                                case SAND:
                                    interleavedData_opaque.push_back(position);
                                    interleavedData_opaque.push_back(normal);
                                    vertexUV.z = 1;
                                    vertexUV.w = 1;
                                    interleavedData_opaque.push_back(vertexUV);
                                    break;
                                case SNOW:
                                    interleavedData_opaque.push_back(position);
                                    interleavedData_opaque.push_back(normal);
                                    vertexUV.z = 1;
                                    vertexUV.w = 1;
                                    interleavedData_opaque.push_back(vertexUV);
                                    break;
                                default:
                                    // Other block types are not yet handled, so we default to debug purple
                                    break;
                                }
                            }
                        }
                        if (t == WATER) {
                            if (neighborType == EMPTY) {
                                m_quatTransparentCount += 1;
                            }
                        } else {
                            m_quatOpaqueCount += 1;
                        }
                    }
                }
            }
        }
    }

    for(int i = 0; i < m_quatTransparentCount; i++) {
        idx_transparent.push_back(i * 4);
        idx_transparent.push_back(i * 4 + 1);
        idx_transparent.push_back(i * 4 + 2);
        idx_transparent.push_back(i * 4);
        idx_transparent.push_back(i * 4 + 2);
        idx_transparent.push_back(i * 4 + 3);

    }
    m_trans = idx_transparent.size();

    for (int i = 0; i < m_quatOpaqueCount; i++) {
        idx_opaque.push_back(i * 4);
        idx_opaque.push_back(i * 4 + 1);
        idx_opaque.push_back(i * 4 + 2);
        idx_opaque.push_back(i * 4);
        idx_opaque.push_back(i * 4 + 2);
        idx_opaque.push_back(i * 4 + 3);
    }
    m_opq = idx_opaque.size();

    m_chunkVBOData.m_trans = interleavedData_transparent;
    m_chunkVBOData.m_op = interleavedData_opaque;
    m_chunkVBOData.m_transIdx = idx_transparent;
    m_chunkVBOData.m_opIdx = idx_opaque;
   // createVBO(interleavedData_transparent, idx_transparent, interleavedData_opaque, idx_opaque);
}

//void Chunk::pushVBO()

void Chunk::createVBO(std::vector<glm::vec4> &interleave_trans, std::vector<GLuint> &idx_trans, std::vector<glm::vec4> &interleave_opq, std::vector<GLuint> &idx_opq) {
    this->m_opq = idx_opq.size();
    this->m_trans = idx_trans.size();

    generateIdx_trans();
    bindIdxTrans();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx_trans);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_trans * sizeof(GLuint), idx_trans.data(), GL_STATIC_DRAW);
    generatedInterleavedTrans();
    bindInterleavedTrans();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_buf_trans);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleave_trans.size() * sizeof(glm::vec4), interleave_trans.data(), GL_STATIC_DRAW);

    generateIdx_opq();
    bindIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx_opq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_opq * sizeof(GLuint), idx_opq.data(), GL_STATIC_DRAW);
    generatedInterleavedOpq();
    bindInterleavedOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_buf_opq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, interleave_opq.size() * sizeof(glm::vec4), interleave_opq.data(), GL_STATIC_DRAW);
}

GLenum Chunk::drawMode() {
    return GL_TRIANGLES;
}

BlockType Chunk::getNeighborBlock(int x,  int y, int z) {
    if(x < 0) {
        if(auto n = m_neighbors[XNEG]) {
            return n->getBlockAt(15, y, z);
        }else {
            return EMPTY;
        }
    }
    if(x > 15) {
        if(auto n = m_neighbors[XPOS]) {
            return n->getBlockAt(0, y, z);
        }else {
            return EMPTY;
        }
    }
    if(z < 0) {
        if(auto n = m_neighbors[ZNEG]) {
            return n->getBlockAt(x, y, 15);
        }else {
            return EMPTY;
        }
    }
    if(z > 15) {
        if(auto n = m_neighbors[ZPOS]) {
            return n->getBlockAt(x, y, 0);
        }else {
            return EMPTY;
        }
    }
    if(y < 0 || y > 255) {
        return EMPTY;
    }
    return getBlockAt(x,y,z);
}

void Chunk::generateTestTerrain(glm::ivec2 chunkPos) {
    for(int x = 0; x < 16; ++x) {
        for(int z = 0; z < 16; ++z) {
            if((x + z) % 2 == 0) {
                setBlockAt(x, 128, z, SNOW);
            }
            else {
                setBlockAt( x, 128, z, SAND);
            }
        }
    }
}




void Chunk::GenerateChunkAt(glm::vec2 xz){

//    int x_end = xz.x+16;
//    int z_end = xz.y+16;

    for(int x=0;x<16;++x){
        for(int z = 0;z<16;++z){
            float worldx = x+xz.x;
            float worldz = z+xz.y;

            float mountainH = mountain(glm::vec2(worldx,worldz));
            float grassH = grass(glm::vec2(worldx,worldz));
            float H = LERP(mountainH,grassH,interpolate(glm::vec2(mountainH,grassH)));

            float moisture = Moisture(worldx,worldz);
//            std::cout<<moisture<<std::endl;

//            BiomeType biome = (H>150) ? Mountain : Grass;

            BiomeType biome = (H>150) ? ((moisture>0.45)? Mountain : Snowland) : ((moisture>0.45)? Grass : Desert);


            for (int i = 0;i<H;i++){
                //set underground
                if (i<=128 && i>0){
                    //float p = perlinNoise3D(glm::vec3(abs((x % 64)/64.f),abs((i%32)/32.f),abs((z%64)/64.f)));

                    setBlockAt(x, i, z, STONE);

//                    if (p>=0){
//                        setBlockAt(x, i, z, STONE);
//                    }else{
//                        if (i<25){
//                            //TODO:replace WATER with LAVA
//                            setBlockAt(x, i, z, LAVA);
//                        }else{
//                            setBlockAt(x, i, z, EMPTY);
//                        }
//                    }
                }
//                else if (i==0){
//                    setBlockAt(x, 0, z, BEDROCK);
//                }
                else{
                    //set y>128
                    switch(biome) {
                    case Grass:
                        setBlockAt(x, i, z, DIRT);
                        break;
                    case Mountain:
                        setBlockAt(x, i, z, STONE);
                        break;
                    case Snowland:
                        setBlockAt(x, i, z, STONE);
                        break;
                    case Desert:
                        setBlockAt(x,i,z,SAND);
                        break;
                    }
                    //set water
                    if (i<139 && getBlockAt(x,i,z) == EMPTY){
                        setBlockAt(x,i,z,WATER);
                    }
                }
            }
            switch(biome) {
            case Grass:
                setBlockAt(x,H,z,GRASS);
                break;
            case Mountain:
                if(H>200){
                    setBlockAt(x,H,z,SNOW);
                }else{
                    setBlockAt(x,H,z,STONE);
                }
                break;
            case Snowland:
                setBlockAt(x, H, z, SNOW);
                break;
            case Desert:
                setBlockAt(x,H,z,SAND);
                break;
            }
        }
    }
}


float interpolate(glm::vec2 uv){
    float t= glm::smoothstep(0.27f, 0.42f,WorleyNoise(glm::vec2(abs(uv.x /2560.f),abs(uv.y/2560.f))));
    return t;
}

float noise2D(glm::vec2 p ) {
    return glm::fract(sin(glm::dot(p, glm::vec2(127.1, 311.7))) *
                 43758.5453);
}
float grass(glm::vec2 pos){
    float h = fbm(cos(pos.x/64.f),cos(pos.y/64.f));
    h = LERP(0.2,0.7,h*10);
    return glm::clamp(h*70+111, 0.f, 255.f);
}
float mountain(glm::vec2 pos){
    float h = WorleyNoise(glm::vec2(cos(pos.x/128.f),cos(pos.y/128.f)));
    h=LERP(50,255,h);
    return glm::clamp(h, 0.f, 255.f);
}



float fbm(float x, float y) {
    float total = 0;
    float persistence = 0.25f;
    int octaves = 8;
    float freq = 2.f;
    float amp = 0.5f;
    for(int i = 1; i <= octaves; i++) {
        freq *= 2.f;
        amp *= persistence;

        total += interpNoise2D(x * freq,
                               y * freq) * amp;
    }
    return total;
}
float interpNoise2D(float x, float y) {
    double intX,intY;
    float fractX = modf(x,&intX);
    float fractY = modf(y,&intY);

    float v1 = noise2D(glm::vec2(intX, intY));
    float v2 = noise2D(glm::vec2(intX+1, intY));
    float v3 = noise2D(glm::vec2(intX, intY+1));
    float v4 = noise2D(glm::vec2(intX+1, intY+1));

    float i1 = LERP(v1, v2, fractX);
    float i2 = LERP(v3, v4, fractX);
    return LERP(i1, i2, fractY);
}
float LERP(float x,float y, float fract){
    return (1-fract)*x+fract*y;
}

float Height(float worldx,float worldz){
    float mountainH = mountain(glm::vec2(worldx,worldz));
    float grassH = grass(glm::vec2(worldx,worldz));
    return LERP(mountainH,grassH,interpolate(glm::vec2(mountainH,grassH)));
}

float Moisture(float worldx,float worldz){
    return WorleyNoise(glm::vec2(abs(worldx/2056.f),abs(worldz/2056.f)));
}

glm::vec2 random2( glm::vec2 p ) {
    return glm::fract(glm::sin(glm::vec2(glm::dot(p, glm::vec2(127.1, 311.7)),
                 glm::dot(p, glm::vec2(269.5,183.3))))
                 * 43758.5453f);
}
float WorleyNoise(glm::vec2 uv) {
    uv *= 10.0; // Now the space is 10x10 instead of 1x1. Change this to any number you want.
    glm::vec2 uvInt = glm::floor(uv);
    glm::vec2 uvFract = glm::fract(uv);
    float minDist = 1.0; // Minimum distance initialized to max.
    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            glm::vec2 neighbor = glm::vec2(float(x), float(y)); // Direction in which neighbor cell lies
            glm::vec2 point = random2(uvInt + neighbor); // Get the Voronoi centerpoint for the neighboring cell
            glm::vec2 diff = neighbor + point - uvFract; // Distance between fragment coord and neighbor’s Voronoi point
            float dist = glm::length(diff);
            minDist = glm::min(minDist, dist);
        }
    }
    return minDist;
}

float perlinNoise3D(glm::vec3 p) {
    float surfletSum = 0.f;
    // Iterate over the 8 integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
            for(int dz = 0; dz <= 1; ++dz) {
                surfletSum += surflet3D(p, glm::floor(p) + glm::vec3(dx, dy, dz));
            }
        }
    }
    return surfletSum;
}
float random3f(glm::vec3 p){
    return glm::fract(sin(glm::dot(p, glm::vec3(127.1, 311.7,212.2))) *
     43758.5453);
}

glm::vec3 random3(glm::vec3 i){
    return glm::vec3(random3f(i),random3f(glm::vec3(i[1],i[2],i[0])),random3f(glm::vec3(i[2],i[0],i[1])));
}

float surflet3D(glm::vec3 p, glm::vec3 gridPoint) {
 // Compute the distance between p and the grid point along each axis, and warp it with a
 // quintic function so we can smooth our cells
     glm::vec3 t2 = glm::abs(p - gridPoint);
     glm::vec3 t = glm::vec3(1.f) - 6.f * glm::vec3(pow(t2[0], 5.f),pow(t2[1], 5.f),pow(t2[2], 5.f)) + 15.f * glm::vec3(pow(t2[0], 4.f),pow(t2[1], 4.f),pow(t2[2], 4.f)) - 10.f * glm::vec3(pow(t2[0], 3.f),pow(t2[1], 3.f),pow(t2[2], 3.f));
     // Get the random vector for the grid point (assume we wrote a function random2
     // that returns a vec2 in the range [0, 1])
     glm::vec3 gradient = random3(gridPoint) * 2.f - glm::vec3(1.f, 1.f, 1.f);
     // Get the vector from the grid point to P
     glm::vec3 diff = p - gridPoint;
     // Get the value of our height field by dotting grid->P with our gradient
     float height = glm::dot(diff, gradient);
     // Scale our height field (i.e. reduce it) by our polynomial falloff function
     return height * t.x * t.y * t.z;
}

