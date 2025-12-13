#include "Octree.h"
#include "Log.h"
#include <algorithm>
#include <limits>


OctreeNode::OctreeNode(const AABB& bounds, int depth, const OctreeConfig& config)
    : bounds(bounds), depth(depth), config(config)
{
    // Initialise the 8 children as nullptr
    for (int i = 0; i < 8; i++)
    {
        children[i] = nullptr;
    }
}

OctreeNode::~OctreeNode()
{
    Clear();
}

void OctreeNode::Insert(GameObject* obj)
{
    if (obj == nullptr) return;

    // If the object does not intersect with this node, we do not insert it.
    if (!IntersectsAABB(obj->globalAABB))
        return;

    // If this node already has children, try to insert into the children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->Insert(obj);
            }
        }
        return;
    }

    // Add the object to this node
    objects.push_back(obj);

    // Check if we need to subdivide
    bool shouldSubdivide = (objects.size() > (size_t)config.maxObjectsPerNode) &&
        (depth < config.maxDepth);

    // Also verify that the node size is sufficient.
    if (shouldSubdivide)
    {
        glm::vec3 size = OctreeUtils::GetAABBSize(bounds);
        float minDimension = glm::min(glm::min(size.x, size.y), size.z);

        if (minDimension < config.minNodeSize)
        {
            shouldSubdivide = false;
        }
    }

    if (shouldSubdivide)
    {
        Subdivide();

        // Redistribute objects to children
        std::vector<GameObject*> tempObjects = objects;
        objects.clear();

        for (GameObject* tempObj : tempObjects)
        {
            bool inserted = false;
            for (int i = 0; i < 8; i++)
            {
                if (children[i] != nullptr && children[i]->IntersectsAABB(tempObj->globalAABB))
                {
                    children[i]->Insert(tempObj);
                    inserted = true;
                }
            }

            // If it does not fit into any child, keep it in this node.
            if (!inserted)
            {
                objects.push_back(tempObj);
            }
        }
    }
}

bool OctreeNode::Remove(GameObject* obj)
{
    if (obj == nullptr) return false;

    // Search in this node
    auto it = std::find(objects.begin(), objects.end(), obj);
    if (it != objects.end())
    {
        objects.erase(it);
        return true;
    }

    // Search in children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr && children[i]->Remove(obj))
            {
                return true;
            }
        }
    }

    return false;
}

void OctreeNode::Clear()
{
    objects.clear();

    for (int i = 0; i < 8; i++)
    {
        children[i].reset();
    }
}

void OctreeNode::Subdivide()
{
    glm::vec3 center = OctreeUtils::GetAABBCenter(bounds);
    glm::vec3 halfSize = OctreeUtils::GetAABBSize(bounds) * 0.5f;

    // Create the 8 octants
    for (int i = 0; i < 8; i++)
    {
        glm::vec3 offset;
        offset.x = (i & 1) ? halfSize.x : -halfSize.x;
        offset.y = (i & 2) ? halfSize.y : -halfSize.y;
        offset.z = (i & 4) ? halfSize.z : -halfSize.z;

        glm::vec3 childCenter = center + offset * 0.5f;

        AABB childBounds;
        childBounds.minPoint = childCenter - halfSize * 0.5f;
        childBounds.maxPoint = childCenter + halfSize * 0.5f;

        children[i] = std::make_unique<OctreeNode>(childBounds, depth + 1, config);
    }
}

int OctreeNode::GetOctant(const AABB& objectAABB) const
{
    glm::vec3 center = OctreeUtils::GetAABBCenter(bounds);
    glm::vec3 objCenter = OctreeUtils::GetAABBCenter(objectAABB);

    int octant = 0;
    if (objCenter.x > center.x) octant |= 1;
    if (objCenter.y > center.y) octant |= 2;
    if (objCenter.z > center.z) octant |= 4;

    return octant;
}

bool OctreeNode::IntersectsAABB(const AABB& aabb) const
{
    return OctreeUtils::AABBIntersects(bounds, aabb);
}

bool OctreeNode::IntersectsRay(const Ray& ray) const
{
    return ray.IntersectsAABB(bounds);
}

void OctreeNode::QueryFrustum(const Frustum& frustum, std::vector<GameObject*>& results)
{
    // If the node does not intersect with the frustum, exit
    if (!frustum.IsAABBInside(bounds.minPoint, bounds.maxPoint))
        return;

    // Add objects from this node
    for (GameObject* obj : objects)
    {
        if (obj != nullptr)
        {
            // Avoid duplicates
            if (std::find(results.begin(), results.end(), obj) == results.end())
            {
                results.push_back(obj);
            }
        }
    }

    // Consult children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->QueryFrustum(frustum, results);
            }
        }
    }
}

void OctreeNode::QueryRay(const Ray& ray, std::vector<GameObject*>& results)
{
    // If the ray does not intersect with the node, exit
    if (!IntersectsRay(ray))
        return;

    // Add objects from this node
    for (GameObject* obj : objects)
    {
        if (obj != nullptr)
        {
            // Avoid duplicates
            if (std::find(results.begin(), results.end(), obj) == results.end())
            {
                results.push_back(obj);
            }
        }
    }

    // Consult children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->QueryRay(ray, results);
            }
        }
    }
}

void OctreeNode::QueryAABB(const AABB& aabb, std::vector<GameObject*>& results)
{
    // If there is no intersection, exit
    if (!IntersectsAABB(aabb))
        return;

    // Add objects from this node that intersect
    for (GameObject* obj : objects)
    {
        if (obj != nullptr && OctreeUtils::AABBIntersects(obj->globalAABB, aabb))
        {
            // Avoid duplicates
            if (std::find(results.begin(), results.end(), obj) == results.end())
            {
                results.push_back(obj);
            }
        }
    }

    // Consult children
    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->QueryAABB(aabb, results);
            }
        }
    }
}

int OctreeNode::GetObjectCount() const
{
    int count = objects.size();

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                count += children[i]->GetObjectCount();
            }
        }
    }

    return count;
}

int OctreeNode::GetTotalNodeCount() const
{
    int count = 1;

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                count += children[i]->GetTotalNodeCount();
            }
        }
    }

    return count;
}

void OctreeNode::GetAllBounds(std::vector<AABB>& outBounds) const
{
    outBounds.push_back(bounds);

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->GetAllBounds(outBounds);
            }
        }
    }
}

void OctreeNode::GetLeafBounds(std::vector<AABB>& outBounds) const
{
    if (IsLeaf())
    {
        outBounds.push_back(bounds);
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->GetLeafBounds(outBounds);
            }
        }
    }
}

Octree::Octree()
{

}

Octree::Octree(const AABB& worldBounds, const OctreeConfig& config)
    : worldBounds(worldBounds), config(config)
{
    root = std::make_unique<OctreeNode>(worldBounds, 0, config);
    LOG("Octree created with bounds: Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)",
        worldBounds.minPoint.x, worldBounds.minPoint.y, worldBounds.minPoint.z,
        worldBounds.maxPoint.x, worldBounds.maxPoint.y, worldBounds.maxPoint.z);
}

Octree::~Octree()
{
    Clear();
}

void Octree::Initialize(const AABB& worldBounds, const OctreeConfig& config)
{
    this->worldBounds = worldBounds;
    this->config = config;

    root = std::make_unique<OctreeNode>(worldBounds, 0, config);
    allObjects.clear();

    LOG("Octree initialized with bounds: Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)",
        worldBounds.minPoint.x, worldBounds.minPoint.y, worldBounds.minPoint.z,
        worldBounds.maxPoint.x, worldBounds.maxPoint.y, worldBounds.maxPoint.z);
}

void Octree::Insert(GameObject* obj)
{
    if (obj == nullptr || root == nullptr) return;

    root->Insert(obj);

    // Keep record for rebuild
    if (std::find(allObjects.begin(), allObjects.end(), obj) == allObjects.end())
    {
        allObjects.push_back(obj);
    }
}

bool Octree::Remove(GameObject* obj)
{
    if (obj == nullptr || root == nullptr) return false;

    bool removed = root->Remove(obj);

    if (removed)
    {
        // Remove from cache
        auto it = std::find(allObjects.begin(), allObjects.end(), obj);
        if (it != allObjects.end())
        {
            allObjects.erase(it);
        }
    }

    return removed;
}

void Octree::Update(GameObject* obj)
{
    if (obj == nullptr) return;

    // Remove and reinsert
    Remove(obj);
    Insert(obj);
}

void Octree::Clear()
{
    if (root != nullptr)
    {
        root->Clear();
    }
    allObjects.clear();
}

void Octree::Rebuild()
{
    if (root == nullptr) return;

    LOG("Rebuilding Octree with %d objects...", (int)allObjects.size());

    // Save objects
    std::vector<GameObject*> tempObjects = allObjects;

    Clear();

    // Reinsert
    for (GameObject* obj : tempObjects)
    {
        if (obj != nullptr)
        {
            Insert(obj);
        }
    }

    LOG("Octree rebuilt. Nodes: %d, Objects: %d", GetNodeCount(), GetObjectCount());
}

std::vector<GameObject*> Octree::QueryFrustum(const Frustum& frustum)
{
    std::vector<GameObject*> results;

    if (root != nullptr)
    {
        root->QueryFrustum(frustum, results);
    }

    return results;
}

std::vector<GameObject*> Octree::QueryRay(const Ray& ray)
{
    std::vector<GameObject*> results;

    if (root != nullptr)
    {
        root->QueryRay(ray, results);
    }

    return results;
}

std::vector<GameObject*> Octree::QueryAABB(const AABB& aabb)
{
    std::vector<GameObject*> results;

    if (root != nullptr)
    {
        root->QueryAABB(aabb, results);
    }

    return results;
}

int Octree::GetObjectCount() const
{
    return root ? root->GetObjectCount() : 0;
}

int Octree::GetNodeCount() const
{
    return root ? root->GetTotalNodeCount() : 0;
}

std::vector<AABB> Octree::GetAllNodeBounds() const
{
    std::vector<AABB> bounds;
    if (root != nullptr)
    {
        root->GetAllBounds(bounds);
    }
    return bounds;
}

std::vector<AABB> Octree::GetLeafNodeBounds() const
{
    std::vector<AABB> bounds;
    if (root != nullptr)
    {
        root->GetLeafBounds(bounds);
    }
    return bounds;
}

namespace OctreeUtils
{
    AABB CalculateSceneBounds(GameObject* root, float padding)
    {
        if (root == nullptr)
        {
            // Return a default AABB
            return AABB(glm::vec3(-50.0f), glm::vec3(50.0f));
        }

        // Initialise with extreme values
        glm::vec3 minPoint(std::numeric_limits<float>::max());
        glm::vec3 maxPoint(std::numeric_limits<float>::lowest());

        bool foundAny = false;

        // Recursive function to process all GameObjects
        std::function<void(GameObject*)> processNode = [&](GameObject* node)
            {
                if (node == nullptr) return;

                // If the object has a valid AABB
                if (node->globalAABB.minPoint != glm::vec3(0.0f) ||
                    node->globalAABB.maxPoint != glm::vec3(0.0f))
                {
                    minPoint = glm::min(minPoint, node->globalAABB.minPoint);
                    maxPoint = glm::max(maxPoint, node->globalAABB.maxPoint);
                    foundAny = true;
                }

                // Process children
                for (const auto& child : node->GetChildren())
                {
                    processNode(child.get());
                }
            };

        processNode(root);

        // If no valid object was found, use default bounds
        if (!foundAny)
        {
            minPoint = glm::vec3(-50.0f);
            maxPoint = glm::vec3(50.0f);
        }

        // Apply padding
        minPoint -= glm::vec3(padding);
        maxPoint += glm::vec3(padding);

        return AABB(minPoint, maxPoint);
    }

    bool AABBIntersects(const AABB& a, const AABB& b)
    {
        return (a.minPoint.x <= b.maxPoint.x && a.maxPoint.x >= b.minPoint.x) &&
            (a.minPoint.y <= b.maxPoint.y && a.maxPoint.y >= b.minPoint.y) &&
            (a.minPoint.z <= b.maxPoint.z && a.maxPoint.z >= b.minPoint.z);
    }

    AABB ExpandAABB(const AABB& aabb, float margin)
    {
        AABB expanded;
        expanded.minPoint = aabb.minPoint - glm::vec3(margin);
        expanded.maxPoint = aabb.maxPoint + glm::vec3(margin);
        return expanded;
    }

    glm::vec3 GetAABBCenter(const AABB& aabb)
    {
        return (aabb.minPoint + aabb.maxPoint) * 0.5f;
    }

    glm::vec3 GetAABBSize(const AABB& aabb)
    {
        return (aabb.maxPoint - aabb.minPoint) * 0.5f;
    }
}
