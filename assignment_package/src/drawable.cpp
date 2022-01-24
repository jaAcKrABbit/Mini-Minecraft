#include "drawable.h"
#include <glm_includes.h>

Drawable::Drawable(OpenGLContext* context)
    : m_count(-1), m_bufIdx(), m_bufPos(), m_bufNor(), m_bufCol(),  m_bufInterleaved(), m_bufUV(),
          m_idxGenerated(false), m_posGenerated(false), m_norGenerated(false), m_colGenerated(false), m_UVGenerated(false),
      m_opq(-1), m_trans(-1),
      m_bufIdx_opq(),
      m_buf_opq(),
      m_bufIdx_trans(),
      m_buf_trans(),
      m_opqidxGenerated(false),
      m_interleaved_opq_Generated(false),
      m_transidxGenerated(false),
      m_interleaved_trans_Generated(false),
      m_interleavedGenerated(false),
      mp_context(context)
{}

Drawable::~Drawable()
{}


void Drawable::destroyVBOdata()
{
    mp_context->glDeleteBuffers(1, &m_bufIdx);
    mp_context->glDeleteBuffers(1, &m_bufPos);
    mp_context->glDeleteBuffers(1, &m_bufNor);
    mp_context->glDeleteBuffers(1, &m_bufCol);
    mp_context->glDeleteBuffers(1, &m_bufUV);
    mp_context->glDeleteBuffers(1, &m_bufInterleaved);
    m_idxGenerated = m_posGenerated = m_norGenerated = m_colGenerated = m_interleavedGenerated = m_UVGenerated = false;
    m_count = -1;

    // for tranparent and opaque
    mp_context->glDeleteBuffers(1, &m_buf_opq);
    mp_context->glDeleteBuffers(1, &m_bufIdx_opq);
    mp_context->glDeleteBuffers(1, &m_buf_trans);
    mp_context->glDeleteBuffers(1, &m_bufIdx_trans);
    m_opqidxGenerated = m_interleaved_opq_Generated = m_transidxGenerated = m_interleaved_trans_Generated = false;
    m_opq = -1;
    m_trans = -1;
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCount()
{
    return m_count;
}

int Drawable::elementTransCount() {
    return m_trans;
}

int Drawable::elementOpqCount() {
    return m_opq;
}

void Drawable::generateIdx()
{
    m_idxGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdx);
}

// for transparent and opaque
void Drawable::generateIdx_trans() {
    m_transidxGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdx_trans);
}

// for transparent and opaque
void Drawable::generateIdx_opq() {
    m_opqidxGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdx_opq);
}

void Drawable::generatePos()
{
    m_posGenerated = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &m_bufPos);
}

void Drawable::generateNor()
{
    m_norGenerated = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufNor);
}

void Drawable::generateCol()
{
    m_colGenerated = true;
    // Create a VBO on our GPU and store its handle in bufCol
    mp_context->glGenBuffers(1, &m_bufCol);
}

void Drawable::generateInterleaved() {
    m_interleavedGenerated = true;
    mp_context->glGenBuffers(1, &m_bufInterleaved);
}

// for transparent and opaque
void Drawable::generatedInterleavedOpq() {
    m_interleaved_opq_Generated = true;
    mp_context->glGenBuffers(1, &m_buf_opq);
}

void Drawable::generatedInterleavedTrans() {
    m_interleaved_trans_Generated = true;
    mp_context->glGenBuffers(1, &m_buf_trans);
}

void Drawable::generateUV()
{
    m_UVGenerated = true;
    // Create a VBO on our GPU and store its handle in bufCol
    mp_context->glGenBuffers(1, &m_bufUV);
}

bool Drawable::bindIdx()
{
    if(m_idxGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    }
    return m_idxGenerated;
}

// for transparent and opq
bool Drawable::bindIdxOpq() {
    if (m_opqidxGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx_opq);
    }
    return m_opqidxGenerated;
}
bool Drawable::bindIdxTrans() {
    if (m_transidxGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx_trans);
    }
    return m_transidxGenerated;
}

bool Drawable::bindPos()
{
    if(m_posGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPos);
    }
    return m_posGenerated;
}


bool Drawable::bindNor()
{
    if(m_norGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufNor);
    }
    return m_norGenerated;
}

bool Drawable::bindCol()
{
    if(m_colGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufCol);
    }
    return m_colGenerated;
}

bool Drawable::bindInterleaved()
{
    if(m_interleavedGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufInterleaved);
    }
    return m_interleavedGenerated;
}

// for transparent and opq
bool Drawable::bindInterleavedTrans()
{
    if (m_interleaved_trans_Generated) {
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_buf_trans);
    }
    return m_interleaved_trans_Generated;
}

bool Drawable::bindInterleavedOpq()
{
    if (m_interleaved_opq_Generated) {
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_buf_opq);
    }
    return m_interleaved_opq_Generated;
}

bool Drawable::bindUV()
{
    if(m_UVGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufUV);
    }
    return m_UVGenerated;
}

InstancedDrawable::InstancedDrawable(OpenGLContext *context)
    : Drawable(context), m_numInstances(0), m_bufPosOffset(-1), m_offsetGenerated(false)
{}

InstancedDrawable::~InstancedDrawable(){}

int InstancedDrawable::instanceCount() const {
    return m_numInstances;
}

void InstancedDrawable::generateOffsetBuf() {
    m_offsetGenerated = true;
    mp_context->glGenBuffers(1, &m_bufPosOffset);
}

bool InstancedDrawable::bindOffsetBuf() {
    if(m_offsetGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufPosOffset);
    }
    return m_offsetGenerated;
}


void InstancedDrawable::clearOffsetBuf() {
    if(m_offsetGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufPosOffset);
        m_offsetGenerated = false;
    }
}
void InstancedDrawable::clearColorBuf() {
    if(m_colGenerated) {
        mp_context->glDeleteBuffers(1, &m_bufCol);
        m_colGenerated = false;
    }
}
