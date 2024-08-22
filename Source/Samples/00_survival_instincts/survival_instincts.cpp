#include "survival_instincts.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Animation.h"
#include "Urho3D/Graphics/AnimationController.h"
#include "Urho3D/Graphics/AnimationState.h"
#include "Urho3D/Graphics/RenderPath.h"
#include "Urho3D/Graphics/Skybox.h"
#include "Urho3D/Graphics/Terrain.h"
#include "Urho3D/GraphicsAPI/Texture3D.h"
#include "Urho3D/Input/Input.h"
#include "Urho3D/Physics/CollisionShape.h"
#include "Urho3D/Physics/PhysicsWorld.h"
#include "Urho3D/Physics/RigidBody.h"
#include "Urho3D/UI/Font.h"
const float CAMERA_MIN_DIST = 1.0f;
const float CAMERA_INITIAL_DIST = 5.0f;
const float CAMERA_MAX_DIST = 20.0f;

void SurvivalInstinctsApplication::Setup()
{
    engineParameters_[EP_WINDOW_WIDTH] = 3000;
    engineParameters_[EP_WINDOW_HEIGHT] = 1600;
    engineParameters_[EP_WINDOW_RESIZABLE] = true;
    engineParameters_[EP_WINDOW_TITLE] = "Survival Instincts";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_HEADLESS]     = false;
    engineParameters_[EP_SOUND]        = false;
}

void SurvivalInstinctsApplication::Start()
{
   CreateScene();

   /// Render da cat
   CreateMainObject();

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
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP | CTRL_PROWL, false);

        // Update controls using keys
        auto* ui = GetSubsystem<UI>();
        if (!ui->GetFocusElement())
        {
           character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown(KEY_W));
           character_->controls_.Set(CTRL_BACK, input->GetKeyDown(KEY_S));
           character_->controls_.Set(CTRL_LEFT, input->GetKeyDown(KEY_A)); ///< change this to controls.yaw
           character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown(KEY_D)); ///< change this to controls.yaw
           character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));
           character_->controls_.Set(CTRL_PROWL, input->GetKeyDown(KEY_SHIFT));
           character_->controls_.Set(CTRL_SPRINT, input->GetKeyDown(KEY_CTRL));
        }

        //todo. When running, make fov higher -- push camera back a bit. Hold shift to walk
        //todo. 8/14 "A" and "D" should actually turn the body mostly, not mouse -- might add a freelook button
        //todo. 8/14 Add camera orbiting
        //todo. 8/14 Add turn speed
        //todo. commenting this line basically enables free-look
//        character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;



        /// Character x rotation
        character_->controls_.yaw_ += (float)input->GetMouseMoveX() * 0.03f;
        character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));


        /// Camera x rotation               Just make it stronger or weaker based on dot product between camera angle and character angle
        Quaternion currentQuat = cameraNode_->GetRotation();
        float dotProduct = currentQuat.DotProduct(character_->GetNode()->GetRotation());
        currentQuat.IncrementYaw((float)input->GetMouseMoveX() * 0.01 * 1/pow(std::abs(dotProduct),17));
        cameraNode_->SetRotation(currentQuat);

        /// Camera position                Just make it stronger or weaker based on dot product between camera angle and character angle
        Vector3 newCameraPosition = character_->GetNode()->GetPosition() + (cameraNode_->GetRotation() * cameraNode_->GetComponent<Camera>()->initialCameraOffset);
        cameraNode_->SetPosition(Vector3(newCameraPosition.x_,character_->GetNode()->GetPosition().y_ + 8.0f,newCameraPosition.z_));


        /// velocity debug
        auto* cache = GetSubsystem<ResourceCache>();
        xVelocityDebugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 23.5);
        xVelocityDebugText_->SetTextAlignment(HA_LEFT);
        xVelocityDebugText_->SetText(String(character_->GetComponent<RigidBody>()->GetLinearVelocity().x_));
        xVelocityDebugText_->SetPosition(ui->GetRoot()->GetWidth() / 15, ui->GetRoot()->GetHeight() / 2.0);

        yVelocityDebugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 23.5);
        yVelocityDebugText_->SetTextAlignment(HA_LEFT);
        yVelocityDebugText_->SetText(String(character_->GetComponent<RigidBody>()->GetLinearVelocity().y_));
        yVelocityDebugText_->SetPosition(ui->GetRoot()->GetWidth() / 15, ui->GetRoot()->GetHeight() / 1.9);

        zVelocityDebugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 23.5);
        zVelocityDebugText_->SetTextAlignment(HA_LEFT);
        zVelocityDebugText_->SetText(String(character_->GetComponent<RigidBody>()->GetLinearVelocity().z_));
        zVelocityDebugText_->SetPosition(ui->GetRoot()->GetWidth() / 15, ui->GetRoot()->GetHeight() / 1.8);

        linearVelocityDebugText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 28.5);
        linearVelocityDebugText_->SetTextAlignment(HA_LEFT);
        linearVelocityDebugText_->SetText(String(sqrt(pow(character_->GetComponent<RigidBody>()->GetLinearVelocity().x_,2) + pow(character_->GetComponent<RigidBody>()->GetLinearVelocity().y_,2) + pow(character_->GetComponent<RigidBody>()->GetLinearVelocity().z_,2))));
        linearVelocityDebugText_->SetPosition(ui->GetRoot()->GetWidth() / 18, ui->GetRoot()->GetHeight() / 2.14);
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
//    cameraNode_->SetPosition(Vector3(2.8f, 8.0f, -31.0f) + characterNode->GetPosition());

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
    ///\todo. pictures of keyboard keys
    ///\todo. text starts off in middle before moving left
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys to move\n");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 16.5);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_LEFT);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_LEFT);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(ui->GetRoot()->GetWidth() / 20, ui->GetRoot()->GetHeight() / 3.0);

    auto* instructionText2 = ui->GetRoot()->CreateChild<Text>();
    instructionText2->SetText("Hold shift to prowl\n");
    instructionText2->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 16.5);
    // The text has multiple rows. Center them in relation to each other
    instructionText2->SetTextAlignment(HA_LEFT);

    // Position the text relative to the screen center
    instructionText2->SetHorizontalAlignment(HA_LEFT);
    instructionText2->SetVerticalAlignment(VA_CENTER);
    instructionText2->SetPosition(ui->GetRoot()->GetWidth() / 20, ui->GetRoot()->GetHeight() / 2.7);

    auto* instructionText3 = ui->GetRoot()->CreateChild<Text>();
    instructionText3->SetText("Hold ctrl to sprint\n");
    instructionText3->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 16.5);
    // The text has multiple rows. Center them in relation to each other
    instructionText3->SetTextAlignment(HA_LEFT);

    // Position the text relative to the screen center
    instructionText3->SetHorizontalAlignment(HA_LEFT);
    instructionText3->SetVerticalAlignment(VA_CENTER);
    instructionText3->SetPosition(ui->GetRoot()->GetWidth() / 20, ui->GetRoot()->GetHeight() / 2.43);

}

void SurvivalInstinctsApplication::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.3f, 0.3f, 0.3f));
    zone->SetFogColor(Color(0.741f, 0.769f, 0.741f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(700.0f);

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

    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
    Node* skyNode = scene_->CreateChild("Sky");
    skyNode->SetScale(500.0f); // The scale actually does not matter
    auto* skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create heightmap terrain
    Node* terrainNode = scene_->CreateChild("Terrain");
    terrainNode->SetPosition(Vector3::ZERO);
    auto* terrain = terrainNode->CreateComponent<Terrain>();
    terrain->SetPatchSize(64);
    terrain->SetSpacing(Vector3(2.0f, 1.0f, 2.0f)); // Spacing between vertices and vertical resolution of the height map
    terrain->SetSmoothing(true);
    terrain->SetHeightMap(cache->GetResource<Image>("Textures/third_terrain.png"));
    terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
    // The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
    // terrain patches and other objects behind it
    terrain->SetOccluder(true);

    auto* body = terrainNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry
    auto* shape = terrainNode->CreateComponent<CollisionShape>();
    shape->SetTerrain();


    // Create the camera. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(1500.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(1.54341388f, 39.6500015f, -50.9499512f));
    cameraNode_->SetRotation(Quaternion(10.0f, 0.0f, 0.0f));

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
    auto* oCache = GetSubsystem<ResourceCache>();

    /// Adjust object node position and size
    Node* objectNode = scene_->CreateChild("Bean");
    objectNode->SetPosition(Vector3(-1.25658607f, 31.65f, -19.9499493f));
    objectNode->SetScale(Vector3(0.047f, 0.047f, 0.047f)); // Scales the model to .25 its original size
//    objectNode->SetScale(Vector3(3.0f, 3.0f, 3.0f));  ///\note. for ninja

    /// Instantiate a child of object node, and adjust it's rotation
    Node* adjustNode = objectNode->CreateChild("AdjNode");
    adjustNode->SetRotation( Quaternion(180, Vector3(1,0,0) ) );
    adjustNode->SetRotation( Quaternion(180, Vector3(0,1,0) ) );

    /// Instantiate a child of adjust node called model object. Create the rendering component + animation controller
    auto* modelObject = adjustNode->CreateComponent<AnimatedModel>();
    modelObject->SetModel(oCache->GetResource<Model>("Models/cat/cat.mdl"));
    modelObject->SetMaterial(oCache->GetResource<Material>("Materials/cat/cat.xml"));

    modelObject->SetCastShadows(true);
    adjustNode->CreateComponent<AnimationController>();

    /// Create rigidbody, and set non-zero mass so that the body becomes dynamic
    auto* body = objectNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);

    /// Set zero angular factor so that physics doesn't turn the character on its own. Instead, we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    /// Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);


    /// Create the character logic component, which takes care of steering the rigidbody
    /// Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    /// and keeps it alive as long as it's not removed from the hierarchy
    character_ = objectNode->CreateComponent<Character>();

    /// Set a capsule shape for collision
    auto* shape = objectNode->CreateComponent<CollisionShape>();
    //    shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f)); /// Probably going to need to adjust this
    Node* characterNode = character_->GetNode();
    shape->SetSphere(20.0f,characterNode->GetPosition()- Vector3(0.0f, 55.0f, 0.0f));

    // Store the initial camera position and orientation relative to the character node
    cameraNode_->GetComponent<Camera>()->initialCameraOffset = (cameraNode_->GetPosition() - objectNode->GetPosition());
    cameraNode_->GetComponent<Camera>()->initialCameraOrientation = (cameraNode_->GetRotation() - objectNode->GetRotation());

}
