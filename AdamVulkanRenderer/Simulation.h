#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include "Manager.h"
#include "Model.h"
#include "Renderer.h"

class Simulation : public Manager
{
public:  
    // Inherited functions
    virtual void Init() override;
    virtual void Update(float dt) override;
    virtual void Destroy() override;

    // TODO: Remove once we have a central messaging system
    void AddGeometryToRenderer(Renderer *renderer);

    // TODO: Move this to another place...probably a new shader object that is created somewhere?
    bool CreateShaders();

//private:
    Model *model;
};

#endif