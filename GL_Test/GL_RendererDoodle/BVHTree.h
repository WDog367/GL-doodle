#pragma once

#include "RenderRaytracer.h"

#include "Primitive.h"

class BVHTree final {
public:
    struct BVHNode
	{
		BVHNode* left = nullptr;
		BVHNode* right = nullptr;
		std::vector<Primitive*> leaves;
		
		NonhierBox bound = NonhierBox();

		BVHNode() {}

		BVHNode(std::vector<Primitive*> &&primitives) :
			left(nullptr), right(nullptr), leaves(primitives) 
		{
			for (auto & leaf : leaves) {
				bound = bound.extend(leaf->getBounds());
			}
		}
		BVHNode(BVHNode* left, BVHNode* right) :
			left(left), right(right), leaves(0) 
		{
			assert(left && right);
			bound = left->bound.extend(right->bound);
		}
		~BVHNode() { delete left; delete right; }
	};

	// struct for holding a primitve and a copy of it's calculated bounds, for aiding in BVH generation
	struct LeafInfo {
		Primitive* primitive;
		NonhierBox bound;
	};


	template <class T>
	BVHTree(std::vector<T> &f) {
		// first generate the leaf nodes
		std::vector<LeafInfo*> leaves(f.size()); // todo: use unique_ptr
		for (int i = 0; i < f.size(); i++) {
			leaves[i] = new LeafInfo{static_cast<Primitive*>(&f[i]), f[i].getBounds()};
		}

		m_root = *generateBVH_Internal(leaves, 0, leaves.size());

		for (int i = 0; i < f.size(); i++) {
			delete leaves[i];
		}
	}

	template <class T>
	BVHTree(std::vector<T*> &f) {
		// first generate the leaf nodes
		std::vector<LeafInfo*> leaves(f.size()); // todo: use unique_ptr
		for (int i = 0; i < f.size(); i++) {
			leaves[i] = new LeafInfo{static_cast<Primitive*>(f[i]), f[i]->getBounds()};
		}

		m_root = *generateBVH_Internal(leaves, 0, leaves.size());

		for (int i = 0; i < f.size(); i++) {
			delete leaves[i];
		}
	}

	~BVHTree() {}

	bool intersect(IntersectResult &out_result, 
				  const Primitive* &out_hitTri,
				  const Ray &r, 
				  double min_t, double max_t);

private:
	BVHNode m_root = BVHNode();

	static bool bvhIntersect(IntersectResult &out_result, const Primitive* &out_hitTri,
				  const glm::vec3 &r, const glm::vec3 &l, 
				  double min_t, double max_t,
				  /*todo: const*/ BVHNode &bvhNode,
				  int &comparisons, int depth = 0);

	BVHTree::BVHNode* generateBVH_Internal(std::vector<BVHTree::LeafInfo*> &leaves, uint32_t begin, uint32_t end);
};