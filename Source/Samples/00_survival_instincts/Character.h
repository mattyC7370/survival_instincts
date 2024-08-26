// Copyright (c) 2008-2023 the Urho3D project
// License: MIT

#pragma once

#include <Urho3D/Input/Controls.h>
#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

const unsigned CTRL_FORWARD = 1;
const unsigned CTRL_BACK = 2;
const unsigned CTRL_LEFT = 4;
const unsigned CTRL_RIGHT = 8;
const unsigned CTRL_JUMP = 16;
const unsigned CTRL_PROWL = 32;
const unsigned CTRL_SPRINT = 64;

const float MOVE_FORCE = 0.8f;
const float INAIR_MOVE_FORCE = 0.002f;
const float JUMP_FORCE = 9.0f;
const float YAW_SENSITIVITY = 0.1f;
const float INAIR_THRESHOLD_TIME = 0.10f;
const float STICK_FORCE = 40.0f;

/// Character component, responsible for physical movement according to controls, as well as animation.
class Character : public LogicComponent
{
    URHO3D_OBJECT(Character, LogicComponent);

public:
    /// Construct.
    explicit Character(Context* context);

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle startup. Called by LogicComponent base class.
    void Start() override;
    /// Handle physics world update. Called by LogicComponent base class.
    void FixedUpdate(float timeStep) override;

    /// Add this method to be called after character creation
    void AdjustRigidBodyProperties();
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);

    /// Movement controls. Assigned by the main program each frame.
    Controls controls_;
    float CalculateUphillAngle();

private:
    /// Handle physics collision event.
    void HandleNodeCollision(StringHash eventType, VariantMap& eventData);

    /// Grounded flag for movement.
    bool onGround_;
    /// Jump flag.
    bool okToJump_;
    /// In air timer. Due to possible physics inaccuracy, character can be off ground for max. 1/10 second and still be allowed to move.
    float inAirTimer_;

    float fBreakForce_{0.03f};  ///< this determines maximum linear velocity on the ground
    float jumpTimer_;


};
