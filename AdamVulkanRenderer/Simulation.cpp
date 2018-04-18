#include "stdafx.h"
#include "Simulation.h"
#include "OBJFile.h"

void Simulation::Init()
{
    model = new Model();

    /// TESTING ZONE! ///
    OBJFile *objFile = new OBJFile();
    objFile->LoadFile("/data/data/com.adam.parker.renderer/cache/box.obj", *model);
    /// END TESTING ZONE! ///   

    // rotation handled in the shader right now...
    model->matrix.m00 = 1;// cos(2.0f*PI);
    model->matrix.m10 = 0;
    model->matrix.m20 = 0;// sin(2.0f*PI);
    model->matrix.m30 = 0;
    model->matrix.m01 = 0;
    model->matrix.m11 = 1;
    model->matrix.m21 = 0;
    model->matrix.m31 = 0;
    model->matrix.m02 = 0;// -sin(2.0f*PI);
    model->matrix.m12 = 0;
    model->matrix.m22 = 1;// cos(2.0f*PI);
    model->matrix.m32 = 0;
    model->matrix.m03 = 0;
    model->matrix.m13 = 0;
    model->matrix.m23 = 0;
    model->matrix.m33 = 1;    

    // Create shaders... TODO: move somewhere else //
    CreateShaders();    
}

void Simulation::Update(float dt)
{

}

// Oh man...this is bad practice
// TODO: Get rid of this when we have a central messaging system
void Simulation::AddGeometryToRenderer(Renderer *renderer)
{
    renderer->AddGeometry(model);
}

void Simulation::Destroy()
{
    delete model;
}

bool Simulation::CreateShaders()
{
    // Make sure these files in assets have "Android Build System" flagged and are included in the build
    //model->shader.gProgram = model->shader.CreateProgramFromFiles("/data/data/com.adam.parker.renderer/cache/vertex.vs", "/data/data/com.adam.parker.renderer/cache/fragment.fs");
    //if (!model->shader.gProgram)
    //{
    //    LOGE("Could not create program.");
    //    return false;
    //}
    //model->shader.gvPositionHandle = glGetAttribLocation(model->shader.gProgram, "vPosition");
    //glInterface::checkGlError("glGetAttribLocation");
    //model->shader.gvNormalHandle = glGetAttribLocation(model->shader.gProgram, "vNormal");
    //glInterface::checkGlError("glGetAttribLocation");
    //model->shader.deltaTimeHandle = glGetUniformLocation(model->shader.gProgram, "deltaTime");
    //glInterface::checkGlError("glGetAttribLocation");
    //model->shader.matrixHandle = glGetUniformLocation(model->shader.gProgram, "model");
    //glInterface::checkGlError("glGetAttribLocation");
    //model->shader.viewHandle = glGetUniformLocation(model->shader.gProgram, "view");
    //glInterface::checkGlError("glGetAttribLocation");
    //model->shader.projectionHandle = glGetUniformLocation(model->shader.gProgram, "projection");
    //LOGI("glGetAttribLocation(\"vPosition\") = %d\n", model->shader.gvPositionHandle);
    //LOGI("glGetAttribLocation(\"vNormal\") = %d\n", model->shader.gvNormalHandle);

	return false;
}