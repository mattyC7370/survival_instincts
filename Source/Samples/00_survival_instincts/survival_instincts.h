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
#include <Urho3D/Scene/SceneEvents.h>
#include "Urho3D/UI/Text.h"
#include "Urho3D/UI/UI.h"
#include "Urho3D/UI/Font.h"

using namespace Urho3D;

class SurvivalInstinctsApplication : public Application
{
public:
    SurvivalInstinctsApplication(Context* context) : Application(context)
    {
        // Register factory and attributes for the Character component, so it can be created via CreateComponent, and loaded / saved
        Character::RegisterObject(context);

        // Construct new Text object, set string to display and font to use
        auto* ui = GetSubsystem<UI>();
        xVelocityDebugText_ = ui->GetRoot()->CreateChild<Text>();
        xVelocityDebugText_->SetTextAlignment(HA_LEFT);
        xVelocityDebugText_->SetColor(Color(1.0f, 0.0f, 0.0f)); // RGB color (Red)

        yVelocityDebugText_ = ui->GetRoot()->CreateChild<Text>();
        yVelocityDebugText_->SetTextAlignment(HA_LEFT);
        yVelocityDebugText_->SetColor(Color(0.0f, 1.0f, 0.0f)); // RGB color (Green)

        zVelocityDebugText_ = ui->GetRoot()->CreateChild<Text>();
        zVelocityDebugText_->SetTextAlignment(HA_LEFT);
        zVelocityDebugText_->SetColor(Color(0.0f, 0.0f, 1.0f)); // RGB color (Blue)

        linearVelocityDebugText_ = ui->GetRoot()->CreateChild<Text>();
        linearVelocityDebugText_->SetTextAlignment(HA_LEFT);
        linearVelocityDebugText_->SetColor(Color(0.65f, 0.4f, 0.65f)); // RGB color (Blue)

        upHillAngleDebugText_ = ui->GetRoot()->CreateChild<Text>();
        upHillAngleDebugText_->SetTextAlignment(HA_LEFT);
        upHillAngleDebugText_->SetColor(Color(0.75f, 0.2f, 0.7f)); // RGB color (Blue)

        sprintSelectionDebugText_ = ui->GetRoot()->CreateChild<Text>();
        sprintSelectionDebugText_->SetTextAlignment(HA_LEFT);
        sprintSelectionDebugText_->SetColor(Color(0.69f, 0.25f, 0.25f)); // RGB color (Blue)

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

    /// Handle application post-update. Update camera position after character has moved.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    Scene* scene_{nullptr};
    Node* cameraNode_{nullptr};

    /// The controllable character component.
    WeakPtr<Character> character_;

    Text* linearVelocityDebugText_{nullptr};
    Text* xVelocityDebugText_{nullptr};
    Text* yVelocityDebugText_{nullptr};
    Text* zVelocityDebugText_{nullptr};
    Text* upHillAngleDebugText_{nullptr};
    Text* sprintSelectionDebugText_{nullptr};
};

URHO3D_DEFINE_APPLICATION_MAIN(SurvivalInstinctsApplication)
