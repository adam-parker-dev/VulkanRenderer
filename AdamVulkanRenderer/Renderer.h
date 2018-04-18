#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <vector>

#include "Manager.h"

#include "Shader.h"
#include "Triangle.h"
#include "Mat4.h"
#include "Vec4.h"
#include "Model.h"

// Renderer class that stores geometry lists and handles render calls.
// The Renderer class is meant to be generic. New classes should be
// made off this to implement specific render pipelines for OpenGL, DirectX,
// Android, PC, etc. These platform combinations will require different
// implementations. No platform-specific code should exist here.
class Renderer : public Manager
{
public:   
    // Inherited functions
    virtual void Init() {}
    virtual void Update(float dt) {}
    virtual void Destroy() {}

    // Renderer-specific functions that can be inherited by renderers
    virtual void AddGeometry(Model *model) {};

protected:
    // View and projection matrices
    Mat4 view;
    Mat4 projection;

    // Geometry list for rendering
    std::vector<Model*> geometryList;

    // Updates based on delta time
    float delta;
    float rotationSpeed = 0.2; // rotations per second
};

#endif