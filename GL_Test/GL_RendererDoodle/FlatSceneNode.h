#pragma once


#include <vector>

#include "Primitive.h"
#include "Scene.h"
#include "BVHTree.h"



class GeometryNode;


// reason to scrap this:
//  it can't be updated ...
//  it needs a pointer back to the geometry node to tell if the transform has changed ..
//  or a search through the list to find 
//  while that is possible, it makes more sense to have a global transform in the geometry node...
//  cool thing here though, is that the BVH tree can be an object in the scene, and the transforms are relative to it
//    it doesn't really matter, the other way, we would need to have all BVHs be in the global space ...
//    question: can't pass local vector into BVH, but can always return vector to global space 
//    at the end of the day, I like having Local-only coordinate spaces, no need to go into messy global space ..
//    so maybe keep it this way, but comment everything better
//    and eithr way, make BVH templated .. there's no reason for transformedPrimitive to implement most of the interface


// transforms primitive into an arbitrary space
// used to collect child-node primitives in the space of a parent-node
class TransformedPrimitive : public Primitive {
    glm::mat4 m_transform;
    glm::mat4 m_inverseTransform;
    const Primitive* m_primitive;

public:
    TransformedPrimitive(const glm::mat4 &transform, const Primitive *primitive) :
        m_transform(transform), m_inverseTransform(glm::inverse(transform)), m_primitive(primitive)
    { }

  bool intersect(IntersectResult &out_result, const Ray &r, float min_t, float max_t) const override 
  {
      Ray r_local = m_inverseTransform * r;

      if (m_primitive->intersect(out_result, r_local, min_t, max_t)) {
          glm::mat3 invTranspose = glm::transpose(glm::mat3(m_inverseTransform));

            out_result.position = glm::vec3(m_transform * glm::vec4(out_result.position, 1.0));
            out_result.normal = glm::vec3(invTranspose * out_result.normal);

            out_result.bitangent = glm::vec3(invTranspose * out_result.bitangent);
            out_result.tangent = glm::vec3(invTranspose * out_result.tangent);
            return true;
      }
      return false;
  }

  NonhierBox getBounds() const override 
  {
      NonhierBox localBounds = m_primitive->getBounds();
      NonhierBox worldBounds;
      
      for (int corner = 0; corner < 8; corner++) {
          glm::vec3 localCorner = localBounds.min() + localBounds.size() * glm::vec3(corner & 0x1, corner & 0x2, corner & 0x4);
          glm::vec3 worldCorner = glm::vec3(m_transform * glm::vec4(localCorner, 1.0));

          worldBounds = worldBounds.extend(worldCorner);
      }
      return worldBounds;
  }  

};

void flattenScene(std::vector<TransformedPrimitive> &out_primitives, std::vector<GeometryNode*> &out_geometryNodes, 
                        SceneNode* node, glm::mat4 parentTransform = glm::mat4(1.0)) {
    glm::mat4 nodeTransform = parentTransform * node->localTransform();

    // todo: maybe handle encountering a BVH node as well...
	if (node->m_nodeType == NodeType::GeometryNode) {
		GeometryNode* geom = static_cast<GeometryNode*>(node);
        out_primitives.emplace_back(nodeTransform, geom->m_primitive);
        out_geometryNodes.push_back(geom);
	}

	for (SceneNode* child : node->children) {
		flattenScene(out_primitives, out_geometryNodes, child, nodeTransform);
	}
}

class FlattenedSceneNode : public SceneNode {
    // TODO: if `BVHTree` was a template class (and we add an intersect function to the geometry node),
    //       we could have a BVHTree of geometry nodes, so this stimultaneus array business wouldn't be needed
    //      (we would have to make copies of all the geometry nodes...)
    std::vector<TransformedPrimitive> m_primitives;
    std::vector<GeometryNode*> m_geometryNodes;

    BVHTree *m_bvhTree = nullptr;

public:
    FlattenedSceneNode(SceneNode *scene) : SceneNode(scene->m_name + " - flattened") {
        m_nodeType = NodeType::FlattenedSceneNode;
        flattenScene(m_primitives, m_geometryNodes, scene);
        m_bvhTree = new BVHTree(m_primitives);
    }

    ~FlattenedSceneNode() {
        delete m_bvhTree;
    }

    bool intersect(IntersectResult &out_hit, const Ray &r, float min_t, float max_t) {
        const Primitive* hit = nullptr;
        if (m_bvhTree->intersect(out_hit, hit, r, min_t, max_t)) {
            const TransformedPrimitive* hitPrimitive = dynamic_cast<const TransformedPrimitive*>(hit);
            assert(hitPrimitive); // the bvh should only have transformed primitives inside, so this cast should work

            uint32_t i = hitPrimitive - &m_primitives[0];
            GeometryNode* hitNode = m_geometryNodes[i];

            out_hit.material = hitNode->m_material;
            return true;
        }
        return false;
    }

};

