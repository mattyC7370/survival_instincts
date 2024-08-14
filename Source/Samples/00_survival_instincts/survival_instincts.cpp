#include "survival_instincts.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/AnimationState.h"
#include "Urho3D/Graphics/RenderPath.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/Physics/CollisionShape.h"
#include "Urho3D/Physics/PhysicsWorld.h"
#include "Urho3D/Physics/RigidBody.h"
#include "Urho3D/UI/Font.h"
#include "Urho3D/UI/Text.h"
#include "Urho3D/UI/UI.h"
const float CAMERA_MIN_DIST = 1.0f;
const float CAMERA_INITIAL_DIST = 5.0f;
const float CAMERA_MAX_DIST = 20.0f;

void SurvivalInstinctsApplication::Setup()
{
    engineParameters_[EP_WINDOW_WIDTH] = 1600;
    engineParameters_[EP_WINDOW_HEIGHT] = 900;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;
    engineParameters_[EP_WINDOW_TITLE] = "Survival Instincts";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_HEADLESS]     = false;
    engineParameters_[EP_SOUND]        = false;
}

void SurvivalInstinctsApplication::Start()
{
   CreateScene();

   // Setup the viewport for displaying the scene
   SetupViewport();

   // On screen prompts
   CreateInstructions();

   // Subscribe to key presses
   SubscribeToEvents();

}

void SurvivalInstinctsApplication::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    auto* input = GetSubsystem<Input>();

    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

        // Update controls using keys
        auto* ui = GetSubsystem<UI>();
        if (!ui->GetFocusElement())
        {
           character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
           character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
           character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A)); ///< change this to controls.yaw
           character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D)); ///< change this to controls.yaw
           character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));
        }

        //todo. 8/14 "A" and "D" should actually turn the body mostly, not mouse -- might add a freelook button
        //todo. commenting this line basically enables free-look
        character_->controls_.yaw_ += (float)input->GetMouseMoveX() * 0.03f;
//        character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;

        //todo. 8/14 convert these to camera controls^^
        Quaternion currentQuat = cameraNode_->GetRotation();
        currentQuat.IncrementYaw((float)input->GetMouseMoveX() * 0.01f);
        cameraNode_->SetRotation(currentQuat);

        // Limit pitch
            //todo. 8/14 dont think I care about setting the character's pitch -- Think this will be automatic
//        character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
        // Set rotation already here so that it's updated every rendering frame instead of every physics frame
        character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));

    }
}

void SurvivalInstinctsApplication::SubscribeToEvents()
{
    // Subscribe to Update event for setting the character controls before physics simulation
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(SurvivalInstinctsApplication, HandleUpdate));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(SurvivalInstinctsApplication, HandlePostUpdate));

    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void SurvivalInstinctsApplication::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    Node* characterNode = character_->GetNode();

    //TODO: better over the shoulder camera
    cameraNode_->SetPosition(Vector3(2.8f, 8.0f, -18.0f) + characterNode->GetPosition());

//    cameraNode_->SetRotation(dir);

    // Get camera lookat dir from character yaw + pitch
//    const Quaternion& rot = characterNode->GetRotation();
//    Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);
//
//    // Third person camera: position behind the character
//    Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 1.7f, 0.0f);
//
//    // Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
//    Vector3 rayDir = dir * Vector3::BACK;
//    // Ensure touch_ is a valid pointer and cameraDistance_ is a member of the type pointed to by touch_
//    float rayDistance = CAMERA_INITIAL_DIST;
//    PhysicsRaycastResult result;
//    scene_->GetComponent<PhysicsWorld>()->RaycastSingle(result, Ray(aimPoint, rayDir), rayDistance, 2);
//    if (result.body_)
//       rayDistance = Min(rayDistance, result.distance_);
//    rayDistance = Clamp(rayDistance, CAMERA_MIN_DIST, CAMERA_MAX_DIST);

//    cameraNode_->SetPosition(aimPoint + rayDir * rayDistance);
//    cameraNode_->SetRotation(dir);

}

void SurvivalInstinctsApplication::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys to move\n");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_LEFT);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);

}

void SurvivalInstinctsApplication::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(50.0f, 1.0f, 50.0f));
    auto* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
    ///turn on collision
    auto* body = planeNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(2);
    auto* shape = planeNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.5f, 0.5f, 0.5f));
    zone->SetFogColor(Color(0.4f, 0.5f, 0.8f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetColor(Color(0.5f, 0.5f, 0.5f));
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 8.0f, -40.0f));
    cameraNode_->SetRotation(Quaternion(10.0f, 0.0f, 0.0f));

    CreateMainObject();
}

void SurvivalInstinctsApplication::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();
    auto* cache = GetSubsystem<ResourceCache>();

    // Set up a viewport to the Renderer subsystem so twhat the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    //Add FXAA (doesn't seem to be working)
//    viewport->GetRenderPath()->Append(cache->GetResource<XMLFile>("PostProcess/Bloom.xml"));
//    viewport->GetRenderPath()->Append(cache->GetResource<XMLFile>("PostProcess/FXAA2.xml"));
//    viewport->GetRenderPath()->SetShaderParameter("BloomMix", Vector2(0.9f, 0.6f));
//    viewport->GetRenderPath()->SetEnabled("Bloom", false);
//    viewport->GetRenderPath()->SetEnabled("FXAA2", false);
//    viewport->SetRenderPath(effectRenderPath);
}

void SurvivalInstinctsApplication::CreateMainObject()
{
    auto* cache = GetSubsystem<ResourceCache>();

    Node* modelNode = scene_->CreateChild("Bean");
    modelNode->SetPosition(Vector3(Random(40.0f) - 23.8f, 0.7f, Random(40.0f) - 20.0f));
    modelNode->SetRotation(Quaternion(0.0f, 180.0f, 0.0f));

    auto* modelObject = modelNode->CreateComponent<AnimatedModel>();
    modelObject->SetModel(cache->GetResource<Model>("Models/cat/cat.mdl"));
    modelNode->SetScale(Vector3(0.025f, 0.025f, 0.025f)); // Scales the model to .25 its original size

    modelObject->SetMaterial(cache->GetResource<Material>("Models/Kachujin/Materials/Kachujin.xml"));
    modelObject->SetCastShadows(true);

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    auto* body = modelNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead, we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    auto* shape = modelNode->CreateComponent<CollisionShape>();
    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f)); /// Probably going to need to adjust this

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    character_ = modelNode->CreateComponent<Character>();

}
