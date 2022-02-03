#include "bullet/btBulletDynamicsCommon.h"
#include <memory>

struct Physics {
    Physics();

    void update(float dt);

    btDbvtBroadphase broadphase;
    btDefaultCollisionConfiguration collisionConfiguration;
    btCollisionDispatcher dispatcher;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld dynamics_world;
};

struct PhysicsEntity : public btMotionState {
    PhysicsEntity(Physics& physics);
    PhysicsEntity(const PhysicsEntity&) = delete;
    PhysicsEntity(PhysicsEntity&& moved);
    ~PhysicsEntity();

    PhysicsEntity& operator=(const PhysicsEntity&) = delete;
    PhysicsEntity& operator=(PhysicsEntity&& moved) = delete;

    void set_mass(float mass);
    void setShape(std::shared_ptr<const btCollisionShape> collisionShape);
    // void setShape(std::shared_ptr<const Mesh> collisionMesh);
    virtual void setWorldTransform(const btTransform& physicsTransform);

    void getWorldTransform(btTransform& physicsTransform) const;

    bool isOnGround() const;

    float mass = 0;
    btDiscreteDynamicsWorld& dynamics_world;
    std::unique_ptr<btRigidBody> body;
    std::shared_ptr<const btCollisionShape> collisionShape;
    // btMotionState motion_state;
};
