#include "Character.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Viewport.h"
#include "Urho3D/Resource/ResourceCache.h"
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;

class SurvivalInstinctsApplication : public Application
{
public:
    SurvivalInstinctsApplication(Context* context) : Application(context)
    {
        // Register factory and attributes for the Character component, so it can be created via CreateComponent, and loaded / saved
        Character::RegisterObject(context);
    }

    // Set window parameters such as window height and window width in pixels, and fullscreen_enable etc..
    void Setup() override;

    // Create scene and setup viewport
    void Start() override;

    void CreateScene();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    void SetupViewport();

    // Make the main character
    void CreateMainObject();

    // Create on screen prompts
    void CreateInstructions();

    // Subscribe to key presses and camera updating
    void SubscribeToEvents();

    void Stop() override
    {
        // Cleanup and shutdown
    }

private:
    /// Handle application update. Set controls to character.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    Scene* scene_{nullptr};
    Node* cameraNode_{nullptr};

    /// The controllable character component.
    WeakPtr<Character> character_;
};

URHO3D_DEFINE_APPLICATION_MAIN(SurvivalInstinctsApplication)
