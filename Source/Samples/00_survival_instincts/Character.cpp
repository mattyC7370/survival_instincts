// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <string>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Math/Ray.h>
#include "Character.h"

Character::Character(Context* context) :
    LogicComponent(context),
    onGround_(false),
    okToJump_(true),
    inAirTimer_(0.0f),
    jumpTimer_(0.0f)  // New timer for jump grace period
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(LogicComponentEvents::FixedUpdate);
}

void Character::RegisterObject(Context* context)
{
    context->RegisterFactory<Character>();

    // These macros register the class attributes to the Context for automatic load / save handling.
    // We specify the Default attribute mode which means it will be used both for saving into file, and network replication
    URHO3D_ATTRIBUTE("Controls Yaw", controls_.yaw_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Controls Pitch", controls_.pitch_, 0.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("On Ground", onGround_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("OK To Jump", okToJump_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("In Air Timer", inAirTimer_, 0.0f, AM_DEFAULT);
}

void Character::Start()
{
    // Component has been inserted into its scene node. Subscribe to events now
    SubscribeToEvent(GetNode(), E_NODECOLLISION, URHO3D_HANDLER(Character, HandleNodeCollision));
}

void Character::FixedUpdate(float timeStep)
{
    auto* body = GetComponent<RigidBody>();
    auto* animCtrl = node_->GetComponent<AnimationController>(true);

    // Update timers
    if (!onGround_)
        inAirTimer_ += timeStep;
    else
        inAirTimer_ = 0.0f;

    if (jumpTimer_ > 0)
        jumpTimer_ -= timeStep;

    bool softGrounded = inAirTimer_ < INAIR_THRESHOLD_TIME;

    // Apply downward force to "stick" the character to the ground
    if (softGrounded)
    {
        body->ApplyForce(Vector3::DOWN * STICK_FORCE);
    }

    // Update movement
    const Quaternion& rot = node_->GetRotation();
    Vector3 moveDir = Vector3::ZERO;
    const Vector3& velocity = body->GetLinearVelocity();
    Vector3 planeVelocity(velocity.x_, 0.0f, velocity.z_);

    // Handle input
    if (controls_.IsDown(CTRL_FORWARD))
        moveDir += Vector3::FORWARD;
    if (controls_.IsDown(CTRL_BACK))
        moveDir += Vector3::BACK;
    if (controls_.IsDown(CTRL_LEFT))
        moveDir += Vector3::LEFT;
    if (controls_.IsDown(CTRL_RIGHT))
        moveDir += Vector3::RIGHT;
    if (controls_.IsDown(CTRL_PROWL))
    {
        fBreakForce_ = 0.12f;
    }
    else
    {
        if (controls_.IsDown(CTRL_SPRINT))
        {
            fBreakForce_ = 0.04f;
        }
        else
        {
            fBreakForce_ = 0.06f;
        }
    }


    // Normalize move vector so that diagonal strafing is not faster
    if (moveDir.LengthSquared() > 0.0f)
        moveDir.Normalize();

    // Calculate slope
    Vector3 slopeNormal = Vector3::UP;
    PhysicsRaycastResult result;
    Ray ray(node_->GetPosition(), Vector3::DOWN);
    GetScene()->GetComponent<PhysicsWorld>()->RaycastSingle(result, ray, 1.5f);
    if (result.body_)
    {
        slopeNormal = result.normal_;
    }
    float slopeAngle = acosf(slopeNormal.DotProduct(Vector3::UP));

    // Calculate the desired velocity
    float desiredSpeed = softGrounded ? MOVE_FORCE / fBreakForce_ : INAIR_MOVE_FORCE / fBreakForce_;
    Vector3 desiredVelocity = rot * moveDir * desiredSpeed;

    // Calculate the velocity difference
    Vector3 velocityDiff = desiredVelocity - planeVelocity;

    // Adjust force based on slope and velocity difference
    float slopeMultiplier = 1.0f + slopeAngle * 2.0f; // Increase force on steeper slopes
    Vector3 force = velocityDiff * (softGrounded ? MOVE_FORCE * slopeMultiplier : INAIR_MOVE_FORCE);

    // Apply movement force
    body->ApplyImpulse(force);

    if (softGrounded)
    {
        // Apply a braking force to limit maximum ground velocity
        Vector3 brakeForce = -planeVelocity * fBreakForce_;
        body->ApplyImpulse(brakeForce);

        // Handle jumping
        if (controls_.IsDown(CTRL_JUMP))
        {
            if (okToJump_)
            {
                body->ApplyImpulse(Vector3::UP * JUMP_FORCE);
                okToJump_ = false;
                jumpTimer_ = 0.2f;
                // TODO: Jump animation
            }
        }
        else
        {
            okToJump_ = true;
        }
    }

    // Reset grounded flag for next frame
    onGround_ = false;
}
void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    // Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
    using namespace NodeCollision;

    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());

    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        /*float contactDistance = */contacts.ReadFloat();
        /*float contactImpulse = */contacts.ReadFloat();

        // If contact is below node center and pointing up, assume it's a ground contact
        if (contactPosition.y_ < (node_->GetPosition().y_ + 1.0f))
        {
            float level = contactNormal.y_;
            if (level > 0.75)
                onGround_ = true;
        }
    }
}

// Add this method to be called after character creation
void Character::AdjustRigidBodyProperties()
{
    auto* body = GetComponent<RigidBody>();
    if (body)
    {
        body->SetMass(1.0f);  // Adjust mass as needed
        body->SetLinearDamping(0.0f);  // Set overall linear damping to 0
        body->SetAngularDamping(0.5f);  // Add some angular damping

        // Allow full movement in all directions
        body->SetLinearFactor(Vector3::ONE);

        body->SetFriction(1.0f);  // Increase friction
        body->SetRollingFriction(1.0f);  // Add rolling friction
        body->SetRestitution(0.0f);  // Remove bounciness

        // Add custom linear velocity damping
        SubscribeToEvent(GetNode(), E_PHYSICSPRESTEP, URHO3D_HANDLER(Character, HandlePhysicsPreStep));
    }

    auto* shape = GetComponent<CollisionShape>();
    if (shape)
    {
        shape->SetMargin(0.01f);  // Reduce collision margin
    }

}

void Character::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    auto* body = GetComponent<RigidBody>();
    if (body && !onGround_ && jumpTimer_ <= 0)  // Apply damping when in air and not during jump grace period
    {
        Vector3 velocity = body->GetLinearVelocity();
        float dampingFactor = 0.1f; // Adjust this value to control the strength of the damping
        float timestep = eventData[PhysicsPreStep::P_TIMESTEP].GetFloat();

        // Apply damping to Y velocity when in air
        velocity.y_ *= (1.0f - dampingFactor * timestep);

        body->SetLinearVelocity(velocity);
    }
}

float Character::CalculateUphillAngle()
{
    auto* physicsWorld = GetScene()->GetComponent<PhysicsWorld>();
    if (!physicsWorld)
    {
        URHO3D_LOGERROR("PhysicsWorld not found");
        return 0.0f;
    }

    Vector3 characterPos = node_->GetPosition();
    Vector3 downDirection = Vector3::DOWN;
    float rayDistance = 1.5f; // Adjust based on your character's height

    PhysicsRaycastResult result;
    physicsWorld->RaycastSingle(result, Ray(characterPos, downDirection), rayDistance);

    if (result.body_)
    {
        Vector3 surfaceNormal = result.normal_;
        float dotProduct = surfaceNormal.DotProduct(Vector3::UP);
        float angleRadians = acosf(dotProduct);
        float angleDegrees = angleRadians * M_RADTODEG;

        // Debug output
        URHO3D_LOGDEBUG("Surface normal: " + surfaceNormal.ToString());
        URHO3D_LOGDEBUG("Uphill angle: " + String(angleDegrees) + " degrees");

        return angleRadians; // Return angle in radians
    }

    // If raycast didn't hit anything, assume flat ground
    return 0.0f;
}