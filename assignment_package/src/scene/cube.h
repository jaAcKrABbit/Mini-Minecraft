#pragma once

#include "drawable.h"
#include <glm_includes.h>
#include "simpledrawable.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Cube : public SimpleInstancedDrawable
{
public:
    Cube(OpenGLContext* context) : SimpleInstancedDrawable(context){}
    virtual ~Cube(){}
    std::vector<glm::vec3> offsets, colors;
    void createVBOdata() override;
    void createInstancedVBOdata(std::vector<glm::vec3> &offsets, std::vector<glm::vec3> &colors) override;
};
