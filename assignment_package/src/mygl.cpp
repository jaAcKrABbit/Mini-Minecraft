#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this), m_sky(this),
      m_texture(nullptr), m_time(0.f),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this), m_progSky(this),
      m_snowInstanced(this),
      m_terrain(this), m_player(glm::vec3(48.f, 156.f, 48.f), m_terrain), previousFrame(QDateTime::currentMSecsSinceEpoch()), currentFrame(QDateTime::currentMSecsSinceEpoch())
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void initialize_offsets_colors(std::vector<glm::vec3> &offsets, std::vector<glm::vec3> &colors)
{
    offsets.push_back(glm::vec3(48, 129, 48));
    colors.push_back(glm::vec3(255.f, 255.f, 255.f) / 255.f);
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glEnable(GL_CULL_FACE);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);
    printGLErrorLog();

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();
    printGLErrorLog();

    //create the sky dome
    m_sky.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
//    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    // sky shader
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
    //    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    printGLErrorLog();
    m_snowInstanced.createSnow(":/glsl/instanced.vert.glsl", ":/glsl/snow.frag.glsl");
    // m_snowInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    printGLErrorLog();
    // m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // Set a color with which to draw geometry.
    // This will ultimately not be used when you change
    // your program to render Chunks with vertex colors
    // and UV coordinates
    //    m_progLambert.setGeometryColor(glm::vec4(0,1,0,1));

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    printGLErrorLog();
    // m_terrain.CreateTestScene();
    m_terrain.CreateSnow();
    m_terrain.initializeSnow();

    // in order for transparency to work properl for WATER blocks
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // load texture from texture pictures
    m_texture = new Texture(this);
    m_texture->create(":/textures/minecraft_textures_all.png");
    m_texture->load(0);
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);
    m_progSky.setViewProjMatrix(glm::inverse(viewproj));
    this->glUniform2i(m_progSky.unifDimensions, width(), height());
    this->glUniform3f(m_progSky.unifEye, m_player.mcr_camera.mcr_position.x,
                     m_player.mcr_camera.mcr_position.y,
                     m_player.mcr_camera.mcr_position.z);

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    //QCursor::setPos(this->mapToGlobal(QPoint(width() / 2.f, height() / 2.f)));
    m_player.mcr_prevPos = m_player.mcr_position;
    this->currentFrame = QDateTime::currentMSecsSinceEpoch();
    float dT = (currentFrame - previousFrame) / 10.f;
    this->m_player.tick(dT, this->m_inputs);
    // reset inputs delta_x and delta_y to be zero
    this->resetMouseDelta();
    m_terrain.updateTerrain(m_player.mcr_position, m_player.mcr_prevPos);
    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
    this->previousFrame = this->currentFrame;
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
//    m_progInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    //sky

    m_progSky.setViewProjMatrix(glm::inverse(m_player.mcr_camera.getViewProj()));
    m_progSky.useMe();
    //camera position is the eye position
    this->glUniform3f(m_progSky.unifEye, m_player.mcr_camera.mcr_position.x,
                    m_player.mcr_camera.mcr_position.y,
                    m_player.mcr_camera.mcr_position.z);
    this->glUniform1f(m_progSky.unifTime,  m_time++);
    m_progSky.draw(m_sky);
    m_snowInstanced.setViewProjMatrix(m_player.mcr_camera.getViewProj());

    m_snowInstanced.setTime(m_time);
    renderTerrain();

    glDisable(GL_DEPTH_TEST);
    m_progLambert.setTime(m_time % 500);
    m_progFlat.setTime(m_time % 500);

    //    m_progInstanced.setTime(m_time);
    m_time++;
    m_progFlat.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progFlat.draw(m_worldAxes);
    // m_snowInstanced.setModelMatrix(glm::mat4());
    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    m_texture->bind(0);
    printGLErrorLog();
    m_terrain.draw(&m_progLambert);
    printGLErrorLog();
 //   m_terrain.drawTransparent(&m_progLambert);
    printGLErrorLog();
    m_terrain.drawSnow(&m_snowInstanced);

}


void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.0f;
    // m_player.mcr_prevPos = m_player.mcr_position;
    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    }
    if (e->key() == Qt::Key_Right) {
        this->m_inputs.rightPressed = true;
    } else if (e->key() == Qt::Key_Left) {
        this->m_inputs.leftPressed = true;
    } else if (e->key() == Qt::Key_Up) {
        this->m_inputs.upPressed = true;
    } else if (e->key() == Qt::Key_Down) {
        this->m_inputs.downPressed = true;
    } else if (e->key() == Qt::Key_W) {
        this->m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        this->m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        this->m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        this->m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        this->m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        this->m_inputs.ePressed = true;
    } else if (e->key() == Qt::Key_Space) {
        this->currentFrame = QDateTime::currentMSecsSinceEpoch();
        this->m_inputs.spacePressed = true;
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_W) {
        this->m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        this->m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_A) {
        this->m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_D) {
        this->m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_E) {
        this->m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_Q) {
        this->m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_Right) {
        this->m_inputs.rightPressed = false;
    } else if (e->key() == Qt::Key_Left) {
        this->m_inputs.leftPressed = false;
    } else if (e->key() == Qt::Key_Down) {
        this->m_inputs.downPressed = false;
    } else if (e->key() == Qt::Key_Up) {
        this->m_inputs.upPressed = false;
    } else if (e->key() == Qt::Key_Space) {
        this->m_inputs.spacePressed = false;
    } else if (e->key() == Qt::Key_F) {
        if (this->m_inputs.flightMode == false) {
            this->m_inputs.flightMode = true;
        } else {
            this->m_inputs.flightMode = false;
        }
    }
}

void MyGL::resetMouseDelta() {
    this->m_inputs.mouseX = 0.f;
    this->m_inputs.mouseY = 0.f;
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    QPoint lastPosition = QPoint(width() / 2.f, height() / 2.f);
    float delta_x = GLfloat(e->x() - lastPosition.x()) / width();
    float delta_y = GLfloat(e->y() - lastPosition.y()) / height();
    float sensitivity = 1.2;
    m_inputs.mouseX = delta_x * sensitivity;
    m_inputs.mouseY = delta_y * sensitivity;
    // move mouse back to center
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2.f, height() / 2.f)));
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        this->m_player.removeBlock(&m_terrain);
    }
    if (e->button() == Qt::RightButton) {
        this->m_player.placeBlock(&m_terrain);
    }
}
