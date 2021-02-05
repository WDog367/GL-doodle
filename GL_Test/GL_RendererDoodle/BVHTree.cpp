// based off implementation in:
// "PBRT Book Third Edition" (Matt Pharr, Wenzel Jakob, Greg Humphreys), 
// retrieved from www.pbr-book.org

#include "BVHTree.h"

#include <memory>
#include <iostream>

#include <glm/glm.hpp>

#include "Primitive.h"


BVHTree::BVHNode* leafNode(std::vector<BVHTree::LeafInfo*> &leaves, uint32_t begin, uint32_t end) {
    std::vector<Primitive*> primitives;
    
    primitives.reserve(end - begin);

    for (int i = begin; i < end; i++) {
        primitives.push_back(leaves[i]->primitive);
    }

    return new BVHTree::BVHNode(std::move(primitives));
}

BVHTree::BVHNode* BVHTree::generateBVH_Internal(std::vector<BVHTree::LeafInfo*> &leaves, uint32_t begin, uint32_t end) {
	assert(end > begin);

	if (end - begin == 1) {
		return leafNode(leaves, begin, end);
	}

	// choose which axis to separate on, separate based on centroid
	glm::vec3 centroidmin = glm::vec3(std::numeric_limits<float>::infinity());
	glm::vec3 centroidmax = glm::vec3(-std::numeric_limits<float>::infinity());

	for (int i = begin; i < end; i++) {
		centroidmax = glm::max(leaves[i]->bound.centroid(), centroidmax);
		centroidmin = glm::min(leaves[i]->bound.centroid(), centroidmin);
	}

	float maxExtent = 0; //epsilon value
	glm::vec3 centroidextent = centroidmax - centroidmin;
	int splitAxis = -1;
	for (int i = 0; i < 3; i++) {
		if (centroidextent[i] > maxExtent) {
			maxExtent = centroidextent[i];
			splitAxis = i;
		}
	}

	if ( splitAxis == -1) {
        return leafNode(leaves, begin, end);
    }

	uint32_t mid = -1;
#if 1
	// separate using surface area hueristic
	const int nBuckets = 12;

	// compute using buckets, rather than comparing all posible separation
	struct Bucket {
		int count = 0;
		NonhierBox bound = NonhierBox({0,0,0}, 0);
	};

	Bucket buckets[nBuckets];
	NonhierBox totalBounds({0,0,0}, 0);

	// put all leaves into buckets
	for (int i = begin; i < end; i++) {
		int b = nBuckets * ((leaves[i]->bound.centroid() - centroidmin) / centroidextent)[splitAxis];
		b = glm::clamp(b, 0, nBuckets - 1);

		buckets[b].bound = buckets[b].bound.extend(leaves[i]->bound);
		buckets[b].count++;

		totalBounds = totalBounds.extend(buckets[b].bound);
	}

	// test the cost of splitting between each bucket
	const float traversalCost = 1.f / 8.f; // ratio of traversal to intersection cost
	float splitCost[nBuckets - 1];

	for (int split = 0; split < nBuckets - 1; split++) {
		Bucket left;
		Bucket right;

		for (int i = 0; i <= split; i++) {
			left.bound = left.bound.extend(buckets[i].bound);
			left.count += buckets[i].count;
		}
		for (int i = split + 1; i < nBuckets; i++) {
			right.bound = right.bound.extend(buckets[i].bound);
			right.count += buckets[i].count;
		}

		splitCost[split] = traversalCost + 
								(left.count * left.bound.surfaceArea() +
								right.count * right.bound.surfaceArea()) /
							totalBounds.surfaceArea();

	}

	// find the min cost
	float minCost = std::numeric_limits<float>::infinity();
	int minCostSplit = -1;
	for ( int i = 0; i < nBuckets - 1; i++) {
		if (splitCost[i] < minCost) {
			minCost = splitCost[i];
			minCostSplit = i;
		}
	}

    // put all children in this node if it costs more than a leaf with them all
    //if (minCost > end - begin) {
    //    return leafNode(leaves, begin, end);
    //}

    // parition the leaves into the 
	LeafInfo** partition = std::partition(&leaves[begin], (&leaves[end - 1]) + 1, 
			[&splitAxis, &nBuckets, &minCostSplit, &centroidmin, &centroidextent](const LeafInfo *elem) { 
				int bucket = nBuckets * ((elem->bound.centroid() - centroidmin) / centroidextent)[splitAxis];
				bucket = glm::clamp(bucket, 0, nBuckets - 1);
				return bucket <= minCostSplit;
			}
		);
	mid = (partition - &leaves[0]);
#else
	// separate using midpoint
	glm::vec3 center = (centroidmin + centroidmax) / 2.f;
	LeafInfo** partition = std::partition(&leaves[begin], (&leaves[end - 1]) + 1,
			[&splitAxis, &center](const LeafInfo *elem) 
			{ return elem->bound.centroid()[splitAxis] < center[splitAxis];});
	mid = (partition - &leaves[0]);
#endif
    if (mid <= begin || mid >= end) {
        // something went wrong with the splitting,
        //  put all of the primitves in a leaf node
        return leafNode(leaves, begin, end);
    }

    return new BVHTree::BVHNode(generateBVH_Internal(leaves, begin, mid), 
                                generateBVH_Internal(leaves, mid, end));
}

bool BVHTree::bvhIntersect(IntersectResult &out_result, const Primitive* &out_hitTri,
				  const glm::vec3 &r, const glm::vec3 &l, 
				  double min_t, double max_t,
				  /*todo: const*/ BVHTree::BVHNode &bvhNode,
				  int &comparisons, int depth) {
	
	comparisons++;

	{
		IntersectResult boundsIntersection;
		if (!bvhNode.bound.contains(r - l*float(min_t)) &&
			!bvhNode.bound.intersect(boundsIntersection, Ray{r, l}, min_t, max_t)) {
			return false;
		}
	}

	if (
			//bvhNode.leaves.size() > 0
			!bvhNode.left && ! bvhNode.right
			) {
		bool bHasIntersection = false;
		for (auto leaf : bvhNode.leaves) {
            if (leaf->intersect(out_result, Ray{r, l}, min_t, max_t)) {
                max_t = out_result.t;
                out_hitTri = leaf;
                bHasIntersection = true;
            }
		}
		return bHasIntersection;
	} else {
		IntersectResult tempResult;
		bool bHasIntersection = false;

		float t_left = std::numeric_limits<float>::infinity(); 
		float t_right = std::numeric_limits<float>::infinity();
		
		if (bvhNode.left->bound.contains(r - l*float(min_t))) {
			t_left = min_t;
		}
		else if (bvhNode.left->bound.intersect(tempResult, Ray{r, l}, min_t, max_t)) {
			t_left = tempResult.t;
		}

		if (bvhNode.right->bound.contains(r - l*float(min_t))) {
			t_right = min_t;
		}
		else if (bvhNode.right->bound.intersect(tempResult, Ray{r, l}, min_t, max_t)) {
			t_right = tempResult.t;
		}

		// test the nearer child first
		BVHNode* first = t_left < t_right ? bvhNode.left : bvhNode.right;
		BVHNode* second = t_left < t_right ? bvhNode.right : bvhNode.left;

		if (bvhIntersect(out_result, out_hitTri, 
			r, l, min_t, max_t, *first,
			comparisons, depth + 1)) {
			max_t = out_result.t;
			bHasIntersection = true;
		}

		if (bvhIntersect(out_result, out_hitTri, 
			r, l, min_t, max_t, *second,
			comparisons, depth + 1)) {
			max_t = out_result.t;
			bHasIntersection = true;
		}

		return bHasIntersection;
	}
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool BVHTree::intersect(IntersectResult &out_result, 
				  const Primitive* &out_hitTri,
				  const Ray &r, 
				  double min_t, double max_t) {
	int comparisons = 0;
	return bvhIntersect(out_result, out_hitTri, r.o, r.l, min_t, max_t, m_root, comparisons);
}