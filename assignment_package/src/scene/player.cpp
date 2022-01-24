#include "player.h"
#include <QString>
#include <iostream>
using namespace glm;

#define EPSILON 0.001

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera), mcr_prevPos(0,0,0)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.
    // First execute rotation
    if (inputs.mouseX != 0.f) {
        float rotation = inputs.mouseX * 180.f;
        if (rotation >= 180.f) {
            rotation -= 180.f;
        } else if (rotation <= -180.f) {
            rotation += 180.f;
        }
        this->rotateOnUpGlobal(-1.f * rotation);
    }
    if (inputs.mouseY != 0.f) {
        float rotation = inputs.mouseY * 180.f;
        if (rotation >= 180.f) {
            rotation -= 180.f;
        } else if (rotation <= -180.f) {
            rotation += 180.f;
        }
        rotation = rotation * -1;
        this->rotateOnRightLocal(rotation);
    }

    if (inputs.flightMode == true) {
        this->flightMode = true;
        // all keys required for flight mode
        if (inputs.wPressed == true) {
            this->m_acceleration = this->acceleration * glm::normalize(this->m_forward);
        } else if (inputs.sPressed == true) {
            this->m_acceleration = this->acceleration * (-1.f) * glm::normalize(this->m_forward);
        } else if (inputs.aPressed == true) {
            this->m_acceleration = this->acceleration * (-1.f) * glm::normalize(this->m_right);
        } else if (inputs.dPressed == true) {
            this->m_acceleration = this->acceleration * glm::normalize(this->m_right);
        } else if (inputs.qPressed == true) {
            this->m_acceleration = this->acceleration * glm::normalize(this->m_up);
        } else if (inputs.ePressed == true) {
            this->m_acceleration = this->acceleration * (-1.f) * glm::normalize(this->m_up);
        } else {
            // reset the velocity
            this->m_velocity = glm::vec3(0.f, 0.f, 0.f);
            this->m_acceleration = glm::vec3(0.f, 0.f, 0.f);
        }
    } else {
        this->flightMode = false;
        if (inputs.wPressed == true) {
            this->m_acceleration = this->acceleration * glm::normalize(glm::vec3(this->m_forward.x, 0.f, this->m_forward.z));
        } else if (inputs.sPressed == true) {
            this->m_acceleration = this->acceleration * (-1.f) * glm::normalize(glm::vec3(this->m_forward.x, 0.f, this->m_forward.z));
        } else if (inputs.aPressed == true) {
            this->m_acceleration = this->acceleration * (-1.f) * glm::normalize(glm::vec3(this->m_right.x, 0.f, this->m_right.z));
        } else if (inputs.dPressed == true) {
            this->m_acceleration = this->acceleration * glm::normalize(glm::vec3(this->m_right.x, 0.f, this->m_right.z));
        } else if (inputs.spacePressed == true) {
            BlockType underPlayer = this->mcr_terrain.getBlockAt(this->m_position.x, this->m_position.y - 0.5, this->m_position.z);
            if (underPlayer != EMPTY) {
                this->m_velocity.y = this->upSpeed;
            } // else do nothing since the character has jumped up
        } else {
            // reset the velocity
            BlockType underPlayer = this->mcr_terrain.getBlockAt(this->m_position.x, this->m_position.y - 0.5, this->m_position.z);
            if (underPlayer == EMPTY) {
                this->m_velocity = glm::vec3(0.f, this->m_velocity.y, 0.f);
                this->m_acceleration = glm::vec3(0.f, -1.f * this->g, 0.f);
            } else {
                this->m_velocity = glm::vec3(0.f, 0.f, 0.f);
                this->m_acceleration = glm::vec3(0.f, 0.f, 0.f);
            }
        }
    }
}

void Player::computePhysics(float dT, const Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    if (this->flightMode == false) {
        BlockType underPlayer = this->mcr_terrain.getBlockAt(this->m_position.x, this->m_position.y - 0.5, this->m_position.z);
        if (dT == 0 && underPlayer == EMPTY) {
            dT = 1.f;
        }
        this->m_velocity *= 0.99;
        this->m_velocity += this->m_acceleration * dT;
        glm::vec3 displacement = this->m_velocity * dT;
        moveWithCollsions(displacement);
    } else {
        this->m_velocity *= 0.99;
        this->m_velocity += this->m_acceleration * dT;
        glm::vec3 displacement = this->m_velocity * dT;
        moveWithCollsions(displacement);
    }
}

// this is from slide 15
bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }

    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

void Player::moveWithCollsions(glm::vec3 rayDirection) {
//    glm::vec3 origin = glm::vec3(this->m_position.x - 0.5f, this->m_position.y, this->m_position.z - 0.5);
//    std::vector<glm::vec3> rayOrigin;
//    for (int i = 0; i <= 1; i++) {
//        for (int k = 0; k >= - 1; k--) {
//            for (int j = 0; j <= 2; j++) {
//                rayOrigin.push_back(origin + glm::vec3(i, j, k));
//            }
//        }
//    }

//    glm::ivec3 out_blockHit = glm::ivec3();
//    float out_dist = 0.f;
//    // we can simplify assumptions, with a length equal to the players' velocity times delta time
//    for (int i = 0; i < rayOrigin.size(); i++) {
//        bool result = gridMarch(rayOrigin[i], rayDirection, this->mcr_terrain, &out_dist, &out_blockHit);
//        if (result == true) {
//            float actualDistance = glm::min(out_dist - 0.005f, glm::length(this->m_position - vec3(out_blockHit)));
//            rayDirection = actualDistance * glm::normalize(rayDirection);
//        }
//    }
    this->moveAlongVector(rayDirection);
    this->m_camera.setPos(this->m_position + (glm::normalize(m_up) * 1.5f));
}

void Player::placeBlock(Terrain* t) {
    glm::vec3 ray_origin = this->m_camera.mcr_position;
    glm::vec3 ray_direction = 3.f * glm::normalize(this->m_forward);
    glm::ivec3 out_blockHit = glm::ivec3();
    float out_dist = 0.f;
    bool result = gridMarch(ray_origin, ray_direction, this->mcr_terrain, &out_dist, &out_blockHit);
    if (result == false) {
        out_blockHit = this->m_camera.mcr_position + 3.f * glm::normalize(this->m_forward);
        Chunk* c = t->getChunkAt(out_blockHit.x, out_blockHit.z).get();
        glm::vec2 chunkOrigin = glm::vec2(floor(out_blockHit.x / 16.f) * 16, floor(out_blockHit.z / 16.f) * 16);

        c->setBlockAt(static_cast<unsigned int>(out_blockHit.x - chunkOrigin.x),
                             static_cast<unsigned int>(out_blockHit.y),
                             static_cast<unsigned int>(out_blockHit.z - chunkOrigin.y), STONE);
        t->getChunkAt(out_blockHit.x, out_blockHit.z).get()->destroyVBOdata();
        t->getChunkAt(out_blockHit.x, out_blockHit.z).get()->createVBOdata();

//        c->setBlockAt()
//        t->setBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z, STONE);
    }
}

void Player::removeBlock(Terrain* t) {
    glm::vec3 ray_origin = this->m_camera.mcr_position;
    glm::vec3 ray_direction = 3.f * glm::normalize(this->m_forward);
    glm::ivec3 out_blockHit = glm::ivec3();
    float out_dist = 0.f;
    bool result = gridMarch(ray_origin, ray_direction, this->mcr_terrain, &out_dist, &out_blockHit);
    if (result == true) {
        t->setBlockAt(out_blockHit.x, out_blockHit.y, out_blockHit.z, EMPTY);
    }
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
