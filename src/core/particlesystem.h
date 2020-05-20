#include "../render/renderresource.h"

class ParticleSystem
{
public:
    explicit ParticleSystem(RenderResource* r)
    {
        renderResource = r;
        name = "";
    }

    void init();
    void update(GameTime& gt);
    Model* getModel()
    {
        return particleSystemModel.get();
    }

private:
    RenderResource* renderResource;

    std::string name;
    std::unique_ptr<Model> particleSystemModel = nullptr;

};