#pragma once

class Manager
{
public:
    virtual void Init() = 0;
    virtual void Update(float dt) = 0;
    virtual void Destroy() = 0;
};