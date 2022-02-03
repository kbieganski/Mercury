#include "physics.h"

Physics::Physics()
    : dispatcher(&collisionConfiguration),
      dynamics_world(&dispatcher, &broadphase, &solver,
                     &collisionConfiguration) {
    dynamics_world.setGravity(btVector3(0, -9.8, 0));
}

void Physics::update(float dt) { dynamics_world.stepSimulation(dt, 1, 0.016); }

using std::make_shared;
using std::make_unique;
using std::move;

PhysicsEntity::PhysicsEntity(Physics& physics)
    : dynamics_world(physics.dynamics_world) {
    collisionShape = make_shared<btEmptyShape>();
    btRigidBody::btRigidBodyConstructionInfo bodyInfo(
        0, this, const_cast<btCollisionShape*>(collisionShape.get()));
    body = make_unique<btRigidBody>(bodyInfo);
    dynamics_world.addRigidBody(body.get());
}

PhysicsEntity::PhysicsEntity(PhysicsEntity&& moved)
    : dynamics_world(moved.dynamics_world) {
    mass = moved.mass;
    collisionShape = move(moved.collisionShape);
    body = move(moved.body);
    body->setMotionState(this);
}

PhysicsEntity::~PhysicsEntity() {
    if (body) {
        dynamics_world.removeRigidBody(body.get());
    }
}

void PhysicsEntity::set_mass(float newMass) {
    mass = newMass;
    dynamics_world.removeRigidBody(body.get());
    btVector3 inertia;
    body->getCollisionShape()->calculateLocalInertia(mass, inertia);
    body->setMassProps(mass, inertia);
    dynamics_world.addRigidBody(body.get());
    body->activate();
}

void PhysicsEntity::setShape(
    std::shared_ptr<const btCollisionShape> collisionShape) {
    dynamics_world.removeRigidBody(body.get());
    this->collisionShape = collisionShape;
    btVector3 inertia;
    collisionShape->calculateLocalInertia(mass, inertia);
    body->setCollisionShape(
        const_cast<btCollisionShape*>(collisionShape.get()));
    body->setMassProps(mass, inertia);
    dynamics_world.addRigidBody(body.get());
    body->activate();
}

void PhysicsEntity::setWorldTransform(const btTransform& physicsTransform) {
    // auto orientation = physicsTransform.getRotation();
    // transform.setLocalOrientation(quat(orientation.w(), orientation.x(),
    // orientation.y(), orientation.z())); auto position =
    // physicsTransform.getOrigin();
    // transform.setLocalPosition(vec3(position.x(), position.y(),
    // position.z()));
}

/*void PhysicsEntity::setShape(std::shared_ptr<const Mesh> collisionMesh) {
    btConvexHullShape tempConvexHull(reinterpret_cast<float*>(vertices.data()),
vertices.size(), sizeof(Vertex)); btShapeHull hull(&tempConvexHull); auto margin
= tempConvexHull.getMargin(); hull.buildHull(margin); convexHull =
make_unique<btConvexHullShape>(reinterpret_cast<const
float*>(hull.getVertexPointer()), hull.numVertices());
    setShape(std::shared_ptr<const btCollisionShape>(collisionMesh,
&collisionMesh->getConvexHull()));
}*/

void PhysicsEntity::getWorldTransform(btTransform& physicsTransform) const {
    physicsTransform.setIdentity();
    // auto orientation = transform.getOrientation();
    // physicsTransform.setRotation(btQuaternion(orientation.x, orientation.y,
    // orientation.z, orientation.w)); auto position = transform.getPosition();
    // physicsTransform.setOrigin(btVector3(position.x, position.y,
    // position.z));
}

bool PhysicsEntity::isOnGround() const {
    // auto position = transform.getPosition();
    // btVector3 aabbMin, aabbMax;
    // body->getAabb(aabbMin, aabbMax);
    // btVector3 from(position.x, position.y, position.z);
    // btVector3 to(position.x, aabbMin.y() - 0.1f, position.z);
    // btDynamicsWorld::ClosestRayResultCallback result(from, to);
    // dynamics_world.rayTest(from, to, result);
    // return result.hasHit();
    return false;
}
