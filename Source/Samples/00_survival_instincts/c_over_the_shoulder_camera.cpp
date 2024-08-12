#include <Urho3D/Urho3DAll.h>

using namespace Urho3D;

class OverTheShoulderCamera : public LogicComponent
{
    URHO3D_OBJECT(OverTheShoulderCamera, LogicComponent);

public:
    OverTheShoulderCamera(Context* context) : LogicComponent(context), cameraNode_(nullptr), targetNode_(nullptr)
    {
        // Set this component to be initialized and updated
        SetUpdateEventMask(LogicComponentEvents::Update);
    }

    static void RegisterObject(Context* context)
    {
        context->RegisterFactory<OverTheShoulderCamera>();
    }

    void Start() override
    {
        // Find the camera node and the target node (player)
        cameraNode_ = GetNode()->GetScene()->GetChild("Camera", true);
        targetNode_ = GetNode()->GetScene()->GetChild("Player", true);

        if (!cameraNode_ || !targetNode_)
        {
            URHO3D_LOGERROR("Camera or Player node not found");
            return;
        }

        // Set initial camera position
        UpdateCameraPosition();
    }

    void Update(float timeStep) override
    {
        // Update camera position each frame
        UpdateCameraPosition();
    }

private:
    void UpdateCameraPosition()
    {
        if (!cameraNode_ || !targetNode_)
            return;

        // Calculate the new camera position
        Vector3 targetPosition = targetNode_->GetPosition();
        Vector3 cameraOffset(0.0f, 5.0f, -10.0f); // Adjust these values as needed
        cameraNode_->SetPosition(targetPosition + cameraOffset);

        // Make the camera look at the target
        cameraNode_->LookAt(targetPosition);
    }

    SharedPtr<Node> cameraNode_;
    SharedPtr<Node> targetNode_;
};

URHO3D_DEFINE_APPLICATION_MAIN()
