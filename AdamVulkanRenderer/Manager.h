#ifndef _MANAGER_H_
#define _MANAGER_H_

class Manager
{
public:
    virtual void Init() = 0;
    virtual void Update(float dt) = 0;
    virtual void Destroy() = 0;
};

#endif