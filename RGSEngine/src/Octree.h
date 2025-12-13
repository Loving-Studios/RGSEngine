#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "GameObject.h"
#include "Frustum.h"
#include "Ray.h"

// Forward declaration
class OctreeNode;

// Octree configuration
struct OctreeConfig
{
    int maxDepth = 6;              // Maximum tree depth
    int maxObjectsPerNode = 8;     // Maximum objects before subdividing
    float minNodeSize = 1.0f;      // Minimum size of a node (prevents infinite subdivision)

    OctreeConfig() = default;
    OctreeConfig(int depth, int objects, float minSize)
        : maxDepth(depth), maxObjectsPerNode(objects), minNodeSize(minSize) {
    }
};

// Octree node
class OctreeNode
{
public:
    OctreeNode(const AABB& bounds, int depth, const OctreeConfig& config);
    ~OctreeNode();

    // Insert a GameObject into the node
    void Insert(GameObject* obj);

    // Remove a GameObject from the node
    bool Remove(GameObject* obj);

    // Clear the node and all its children
    void Clear();

    // Query: Get all objects that intersect with the frustum
    void QueryFrustum(const Frustum& frustum, std::vector<GameObject*>& results);

    // Query: Get all objects that intersect with a ray
    void QueryRay(const Ray& ray, std::vector<GameObject*>& results);

    // Query: Get all objects in an AABB
    void QueryAABB(const AABB& aabb, std::vector<GameObject*>& results);

    // Obtain the AABB of the node
    const AABB& GetBounds() const { return bounds; }

    // Debug information
    int GetObjectCount() const;
    int GetTotalNodeCount() const;
    int GetDepth() const { return depth; }
    bool IsLeaf() const { return children[0] == nullptr; }

    // For debug display
    void GetAllBounds(std::vector<AABB>& outBounds) const;
    void GetLeafBounds(std::vector<AABB>& outBounds) const;

private:
    AABB bounds;
    int depth;
    OctreeConfig config;

    std::vector<GameObject*> objects;// Objects in this node
    std::unique_ptr<OctreeNode> children[8];// 8 children (octuplets)

    // Subdivide the node into 8 octants
    void Subdivide();

    // Determine in which octant(s) an object should be
    int GetOctant(const AABB& objectAABB) const;

    // Check if an AABB intersects with this node
    bool IntersectsAABB(const AABB& aabb) const;

    // Check if a ray intersects with this node
    bool IntersectsRay(const Ray& ray) const;
};

// Octree main class
class Octree
{
public:
    Octree();
    Octree(const AABB& worldBounds, const OctreeConfig& config = OctreeConfig());
    ~Octree();

    // Initialise or reinitialise the Octree
    void Initialize(const AABB& worldBounds, const OctreeConfig& config = OctreeConfig());


    void Insert(GameObject* obj);


    bool Remove(GameObject* obj);

   
    void Update(GameObject* obj);

    
    void Clear();

 
    void Rebuild();


    // Get visible objects in the frustum
    std::vector<GameObject*> QueryFrustum(const Frustum& frustum);

    // Obtain objects that intersect with a ray
    std::vector<GameObject*> QueryRay(const Ray& ray);

    // Get objects in an area (AABB)
    std::vector<GameObject*> QueryAABB(const AABB& aabb);

    int GetObjectCount() const;

    int GetNodeCount() const;

    int GetMaxDepth() const { return config.maxDepth; }

    // For display purposes
    std::vector<AABB> GetAllNodeBounds() const;
    std::vector<AABB> GetLeafNodeBounds() const;

    const OctreeNode* GetRoot() const { return root.get(); }

    bool IsInitialized() const { return root != nullptr; }

private:
    std::unique_ptr<OctreeNode> root;
    OctreeConfig config;
    AABB worldBounds;

    // Object cache for rebuild
    std::vector<GameObject*> allObjects;
};

namespace OctreeUtils
{
    AABB CalculateSceneBounds(GameObject* root, float padding = 10.0f);

    bool AABBIntersects(const AABB& a, const AABB& b);

    AABB ExpandAABB(const AABB& aabb, float margin);

    glm::vec3 GetAABBCenter(const AABB& aabb);

    glm::vec3 GetAABBSize(const AABB& aabb);
}