// SecretEngine - New ECS System Example
// This demonstrates the Overload-inspired architecture

#include <SecretEngine/Scene.h>
#include <SecretEngine/EntityObject.h>
#include <SecretEngine/ComponentsNew.h>
#include <SecretEngine/Event.h>
#include <iostream>

using namespace SecretEngine;

// Example: Custom component with lifecycle
class RotatorComponent : public IComponent {
public:
    explicit RotatorComponent(EntityObject& owner, float speed = 45.0f)
        : IComponent(owner)
        , m_rotationSpeed(speed)
    {}
    
    COMPONENT_TYPE(RotatorComponent)
    
    void OnStart() override {
        std::cout << "RotatorComponent started on " << m_owner.GetName() << std::endl;
    }
    
    void OnUpdate(float deltaTime) override {
        // Rotate the entity
        auto* transform = m_owner.GetComponent<TransformComponentNew>();
        if (transform) {
            Vec3 rotation = transform->GetLocalRotation();
            rotation.y += m_rotationSpeed * deltaTime;
            transform->SetLocalRotation(rotation);
        }
    }
    
    void SetSpeed(float speed) { m_rotationSpeed = speed; }
    float GetSpeed() const { return m_rotationSpeed; }
    
private:
    float m_rotationSpeed;
};

// Example: Using events
void EventExample() {
    std::cout << "\n=== Event System Example ===" << std::endl;
    
    Event<int, std::string> onScoreChanged;
    
    // Add listeners
    auto listener1 = onScoreChanged.AddListener([](int score, std::string player) {
        std::cout << "Listener 1: " << player << " scored " << score << " points!" << std::endl;
    });
    
    auto listener2 = onScoreChanged.AddListener([](int score, std::string player) {
        std::cout << "Listener 2: Total score is now " << score << std::endl;
    });
    
    // Invoke event
    onScoreChanged.Invoke(100, "Player1");
    onScoreChanged(200, "Player2"); // Alternative syntax
    
    // Remove a listener
    onScoreChanged.RemoveListener(listener1);
    onScoreChanged.Invoke(300, "Player3"); // Only listener2 will be called
}

// Example: Entity hierarchy
void HierarchyExample() {
    std::cout << "\n=== Entity Hierarchy Example ===" << std::endl;
    
    Scene scene("Test Scene");
    
    // Create parent entity
    auto* parent = scene.CreateEntity("Parent");
    auto* parentTransform = parent->AddComponent<TransformComponentNew>();
    parentTransform->SetLocalPosition(Vec3(0, 0, 0));
    
    // Create child entity
    auto* child = scene.CreateEntity("Child");
    auto* childTransform = child->AddComponent<TransformComponentNew>();
    childTransform->SetLocalPosition(Vec3(5, 0, 0));
    child->SetParent(parent);
    
    // Child's world position should be parent + local
    Vec3 childWorldPos = childTransform->GetWorldPosition();
    std::cout << "Child world position: (" 
              << childWorldPos.x << ", " 
              << childWorldPos.y << ", " 
              << childWorldPos.z << ")" << std::endl;
    
    // Move parent, child should follow
    parentTransform->SetLocalPosition(Vec3(10, 0, 0));
    childWorldPos = childTransform->GetWorldPosition();
    std::cout << "After moving parent, child world position: (" 
              << childWorldPos.x << ", " 
              << childWorldPos.y << ", " 
              << childWorldPos.z << ")" << std::endl;
}

// Example: Component lifecycle
void LifecycleExample() {
    std::cout << "\n=== Component Lifecycle Example ===" << std::endl;
    
    Scene scene("Lifecycle Test");
    
    // Create entity with components
    auto* entity = scene.CreateEntity("TestEntity");
    entity->AddComponent<TransformComponentNew>();
    entity->AddComponent<MeshComponentNew>();
    auto* rotator = entity->AddComponent<RotatorComponent>(90.0f);
    
    std::cout << "Entity created with components" << std::endl;
    
    // Start the scene (triggers OnAwake, OnEnable, OnStart)
    scene.Play();
    
    // Simulate a few frames
    for (int i = 0; i < 3; i++) {
        std::cout << "\nFrame " << i << ":" << std::endl;
        scene.Update(0.016f); // ~60 FPS
    }
    
    // Deactivate entity (triggers OnDisable)
    std::cout << "\nDeactivating entity..." << std::endl;
    entity->SetActive(false);
    
    // Reactivate (triggers OnEnable)
    std::cout << "\nReactivating entity..." << std::endl;
    entity->SetActive(true);
    
    // Stop scene (triggers OnDisable)
    std::cout << "\nStopping scene..." << std::endl;
    scene.Stop();
}

// Example: Fast access for rendering
void FastAccessExample() {
    std::cout << "\n=== Fast Access Example ===" << std::endl;
    
    Scene scene("Rendering Test");
    
    // Create multiple entities with different components
    for (int i = 0; i < 5; i++) {
        auto* entity = scene.CreateEntity("Entity_" + std::to_string(i));
        entity->AddComponent<TransformComponentNew>();
        
        if (i % 2 == 0) {
            auto* mesh = entity->AddComponent<MeshComponentNew>();
            mesh->SetMeshPath("cube.meshbin");
        }
        
        if (i == 0) {
            auto* camera = entity->AddComponent<CameraComponentNew>();
            camera->SetMainCamera(true);
        }
        
        if (i == 1 || i == 3) {
            auto* light = entity->AddComponent<LightComponentNew>();
            light->SetType(LightComponentNew::LightType::Point);
        }
    }
    
    // Access components efficiently without iterating all entities
    const auto& fastAccess = scene.GetFastAccess();
    
    std::cout << "Scene contains:" << std::endl;
    std::cout << "  - " << fastAccess.transforms.size() << " transforms" << std::endl;
    std::cout << "  - " << fastAccess.meshes.size() << " meshes" << std::endl;
    std::cout << "  - " << fastAccess.cameras.size() << " cameras" << std::endl;
    std::cout << "  - " << fastAccess.lights.size() << " lights" << std::endl;
    
    // Find main camera
    auto* mainCamera = scene.FindMainCamera();
    if (mainCamera) {
        std::cout << "Main camera found on entity: " 
                  << mainCamera->GetOwner().GetName() << std::endl;
    }
}

// Example: Entity queries
void QueryExample() {
    std::cout << "\n=== Entity Query Example ===" << std::endl;
    
    Scene scene("Query Test");
    
    // Create entities with tags
    auto* player = scene.CreateEntity("Player");
    player->SetTag("Player");
    
    auto* enemy1 = scene.CreateEntity("Enemy1");
    enemy1->SetTag("Enemy");
    
    auto* enemy2 = scene.CreateEntity("Enemy2");
    enemy2->SetTag("Enemy");
    
    auto* prop = scene.CreateEntity("Prop");
    prop->SetTag("Environment");
    
    // Query by name
    auto* foundPlayer = scene.FindEntityByName("Player");
    if (foundPlayer) {
        std::cout << "Found entity by name: " << foundPlayer->GetName() << std::endl;
    }
    
    // Query by tag
    auto enemies = scene.FindEntitiesByTag("Enemy");
    std::cout << "Found " << enemies.size() << " enemies" << std::endl;
    for (auto* enemy : enemies) {
        std::cout << "  - " << enemy->GetName() << std::endl;
    }
}

int main() {
    std::cout << "SecretEngine - New ECS System Examples\n" << std::endl;
    
    EventExample();
    HierarchyExample();
    LifecycleExample();
    FastAccessExample();
    QueryExample();
    
    std::cout << "\n=== All examples completed ===" << std::endl;
    return 0;
}
