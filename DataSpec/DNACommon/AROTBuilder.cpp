#include "AROTBuilder.hpp"

#include <algorithm>
#include <array>

#include "hecl/Blender/Connection.hpp"
#include "PATH.hpp"

namespace DataSpec {
logvisor::Module Log("AROTBuilder");

constexpr s32 AROT_MAX_LEVEL = 10;
constexpr s32 AROT_MIN_MODELS = 8;
constexpr s32 COLLISION_MIN_NODE_TRIANGLES = 8;
constexpr s32 PATH_MIN_NODE_REGIONS = 16;
constexpr float AROT_MIN_SUBDIV = 10.f;

static zeus::CAABox SplitAABB(const zeus::CAABox& aabb, int i) {
  zeus::CAABox pos, neg;
  aabb.splitZ(neg, pos);
  if (i & 4) {
    zeus::CAABox(pos).splitY(neg, pos);
    if (i & 2) {
      zeus::CAABox(pos).splitX(neg, pos);
      if (i & 1)
        return pos;
      else
        return neg;
    } else {
      zeus::CAABox(neg).splitX(neg, pos);
      if (i & 1)
        return pos;
      else
        return neg;
    }
  } else {
    zeus::CAABox(neg).splitY(neg, pos);
    if (i & 2) {
      zeus::CAABox(pos).splitX(neg, pos);
      if (i & 1)
        return pos;
      else
        return neg;
    } else {
      zeus::CAABox(neg).splitX(neg, pos);
      if (i & 1)
        return pos;
      else
        return neg;
    }
  }
}

void AROTBuilder::Node::mergeSets(int a, int b) {
  childNodes[a].childIndices.insert(childNodes[b].childIndices.cbegin(), childNodes[b].childIndices.cend());
  childNodes[b].childIndices = childNodes[a].childIndices;
}

bool AROTBuilder::Node::compareSets(int a, int b) const {
  return childNodes[a].childIndices != childNodes[b].childIndices;
}

void AROTBuilder::Node::addChild(int level, int minChildren, const std::vector<zeus::CAABox>& triBoxes,
                                 const zeus::CAABox& curAABB, BspNodeType& typeOut) {
  /* Gather intersecting faces */
  for (size_t i = 0; i < triBoxes.size(); ++i)
    if (triBoxes[i].intersects(curAABB))
      childIndices.insert(i);

  zeus::CVector3f extents = curAABB.extents();

  /* Return early if empty, triangle intersection below performance threshold, or at max level */
  if (childIndices.empty()) {
    typeOut = BspNodeType::Invalid;
    return;
  } else if (childIndices.size() < minChildren || level == AROT_MAX_LEVEL ||
             std::max(extents.x(), std::max(extents.y(), extents.z())) < AROT_MIN_SUBDIV) {
    typeOut = BspNodeType::Leaf;
    return;
  }

  /* Subdivide */
  typeOut = BspNodeType::Branch;
  childNodes.resize(8);
  for (int i = 0; i < 8; ++i) {
    BspNodeType chType;
    childNodes[i].addChild(level + 1, minChildren, triBoxes, SplitAABB(curAABB, i), chType);
    flags |= int(chType) << (i * 2);
  }

  /* Unsubdivide minimum axis dimensions */
  if (extents.x() < AROT_MIN_SUBDIV) {
    mergeSets(0, 1);
    mergeSets(4, 5);
    mergeSets(2, 3);
    mergeSets(6, 7);
  }
  if (extents.y() < AROT_MIN_SUBDIV) {
    mergeSets(0, 2);
    mergeSets(1, 3);
    mergeSets(4, 6);
    mergeSets(5, 7);
  }
  if (extents.z() < AROT_MIN_SUBDIV) {
    mergeSets(0, 4);
    mergeSets(1, 5);
    mergeSets(2, 6);
    mergeSets(3, 7);
  }

  /* Unsubdivide */
  compSubdivs = 0;
  if (compareSets(0, 1) || compareSets(4, 5) || compareSets(2, 3) || compareSets(6, 7))
    compSubdivs |= 0x1;
  if (compareSets(0, 2) || compareSets(1, 3) || compareSets(4, 6) || compareSets(5, 7))
    compSubdivs |= 0x2;
  if (compareSets(0, 4) || compareSets(1, 5) || compareSets(2, 6) || compareSets(3, 7))
    compSubdivs |= 0x4;

  if (!compSubdivs) {
    typeOut = BspNodeType::Leaf;
    childNodes = std::vector<Node>();
    flags = 0;
  }
}

size_t AROTBuilder::BitmapPool::addIndices(const std::set<int>& indices) {
  for (size_t i = 0; i < m_pool.size(); ++i)
    if (m_pool[i] == indices)
      return i;
  m_pool.push_back(indices);
  return m_pool.size() - 1;
}

constexpr std::array<uint32_t, 8> AROTChildCounts{
    0, 2, 2, 4, 2, 4, 4, 8,
};

void AROTBuilder::Node::nodeCount(size_t& sz, size_t& idxRefs, BitmapPool& bmpPool, size_t& curOff) {
  sz += 1;
  poolIdx = bmpPool.addIndices(childIndices);
  if (poolIdx > 65535)
    Log.report(logvisor::Fatal, FMT_STRING("AROT bitmap exceeds 16-bit node addressing; area too complex"));

  uint32_t childCount = AROTChildCounts[compSubdivs];
  nodeOff = curOff;
  nodeSz = childCount * 2 + 4;
  curOff += nodeSz;
  if (childNodes.size()) {
    for (int k = 0; k < 1 + ((compSubdivs & 0x4) != 0); ++k) {
      for (int j = 0; j < 1 + ((compSubdivs & 0x2) != 0); ++j) {
        for (int i = 0; i < 1 + ((compSubdivs & 0x1) != 0); ++i) {
          int idx = k * 4 + j * 2 + i;
          childNodes[idx].nodeCount(sz, idxRefs, bmpPool, curOff);
        }
      }
    }
    idxRefs += childCount;
  }
}

void AROTBuilder::Node::writeIndirectionTable(athena::io::MemoryWriter& w) {
  w.writeUint32Big(nodeOff);
  if (childNodes.size()) {
    for (int k = 0; k < 1 + ((compSubdivs & 0x4) != 0); ++k) {
      for (int j = 0; j < 1 + ((compSubdivs & 0x2) != 0); ++j) {
        for (int i = 0; i < 1 + ((compSubdivs & 0x1) != 0); ++i) {
          int idx = k * 4 + j * 2 + i;
          childNodes[idx].writeIndirectionTable(w);
        }
      }
    }
  }
}

void AROTBuilder::Node::writeNodes(athena::io::MemoryWriter& w, int nodeIdx) {
  w.writeUint16Big(poolIdx);
  w.writeUint16Big(compSubdivs);

  if (childNodes.size()) {
    int curIdx = nodeIdx + 1;
    if (curIdx > 65535)
      Log.report(logvisor::Fatal, FMT_STRING("AROT node exceeds 16-bit node addressing; area too complex"));

    std::array<int, 8> childIndices;

    for (int k = 0; k < 1 + ((compSubdivs & 0x4) != 0); ++k) {
      for (int j = 0; j < 1 + ((compSubdivs & 0x2) != 0); ++j) {
        for (int i = 0; i < 1 + ((compSubdivs & 0x1) != 0); ++i) {
          int idx = k * 4 + j * 2 + i;
          w.writeUint16Big(curIdx);
          childIndices[idx] = curIdx;
          childNodes[idx].advanceIndex(curIdx);
        }
      }
    }

    for (int k = 0; k < 1 + ((compSubdivs & 0x4) != 0); ++k) {
      for (int j = 0; j < 1 + ((compSubdivs & 0x2) != 0); ++j) {
        for (int i = 0; i < 1 + ((compSubdivs & 0x1) != 0); ++i) {
          int idx = k * 4 + j * 2 + i;
          childNodes[idx].writeNodes(w, childIndices[idx]);
        }
      }
    }
  }
}

void AROTBuilder::Node::advanceIndex(int& nodeIdx) {
  ++nodeIdx;
  if (childNodes.size()) {
    for (int k = 0; k < 1 + ((compSubdivs & 0x4) != 0); ++k) {
      for (int j = 0; j < 1 + ((compSubdivs & 0x2) != 0); ++j) {
        for (int i = 0; i < 1 + ((compSubdivs & 0x1) != 0); ++i) {
          int idx = k * 4 + j * 2 + i;
          childNodes[idx].advanceIndex(nodeIdx);
        }
      }
    }
  }
}

void AROTBuilder::Node::colSize(size_t& totalSz) {
  if (childIndices.size()) {
    nodeOff = totalSz;
    if (childNodes.empty()) {
      totalSz += 26 + childIndices.size() * 2;
    } else {
      totalSz += 36;
      for (int i = 0; i < 8; ++i)
        childNodes[i].colSize(totalSz);
    }
  }
}

void AROTBuilder::Node::writeColNodes(uint8_t*& ptr, const zeus::CAABox& curAABB) {
  if (childIndices.size()) {
    if (childNodes.empty()) {
      float* aabbOut = reinterpret_cast<float*>(ptr);
      aabbOut[0] = hecl::SBig(curAABB.min[0]);
      aabbOut[1] = hecl::SBig(curAABB.min[1]);
      aabbOut[2] = hecl::SBig(curAABB.min[2]);
      aabbOut[3] = hecl::SBig(curAABB.max[0]);
      aabbOut[4] = hecl::SBig(curAABB.max[1]);
      aabbOut[5] = hecl::SBig(curAABB.max[2]);
      athena::io::MemoryWriter w(ptr + 24, INT32_MAX);
      w.writeUint16Big(childIndices.size());
      for (int idx : childIndices)
        w.writeUint16Big(idx);
      ptr += 26 + childIndices.size() * 2;
    } else {
      uint16_t* pflags = reinterpret_cast<uint16_t*>(ptr);
      uint32_t* offsets = reinterpret_cast<uint32_t*>(ptr + 4);
      memset(pflags, 0, sizeof(uint32_t) * 9);
      for (int i = 0; i < 8; ++i) {
        const Node& chNode = childNodes[i];
        BspNodeType type = BspNodeType((flags >> (i * 2)) & 0x3);
        if (type != BspNodeType::Invalid)
          offsets[i] = hecl::SBig(uint32_t(chNode.nodeOff - nodeOff - 36));
      }

      *pflags = hecl::SBig(flags);
      ptr += 36;

      for (int i = 0; i < 8; ++i)
        childNodes[i].writeColNodes(ptr, SplitAABB(curAABB, i));
    }
  }
}

void AROTBuilder::Node::pathCountNodesAndLookups(size_t& nodeCount, size_t& lookupCount) {
  ++nodeCount;
  if (childNodes.empty()) {
    lookupCount += childIndices.size();
  } else {
    for (int i = 0; i < 8; ++i)
      childNodes[i].pathCountNodesAndLookups(nodeCount, lookupCount);
  }
}

template <class PAKBridge>
void AROTBuilder::Node::pathWrite(DNAPATH::PATH<PAKBridge>& path, const zeus::CAABox& curAABB) {
  if (childNodes.empty()) {
    auto& n = path.octree.emplace_back();
    n.isLeaf = 1;
    n.aabb[0] = curAABB.min;
    n.aabb[1] = curAABB.max;
    n.centroid = curAABB.center();
    std::fill(std::begin(n.children), std::end(n.children), 0xFFFFFFFF);
    n.regionCount = childIndices.size();
    n.regionStart = path.octreeRegionLookup.size();
    for (int r : childIndices)
      path.octreeRegionLookup.push_back(r);
  } else {
    std::array<atUint32, 8> children;
    for (size_t i = 0; i < children.size(); ++i) {
      /* Head recursion (first node will be a leaf) */
      childNodes[i].pathWrite(path, SplitAABB(curAABB, static_cast<int>(i)));
      children[i] = path.octree.size() - 1;
    }

    auto& n = path.octree.emplace_back();
    n.isLeaf = 0;
    n.aabb[0] = curAABB.min;
    n.aabb[1] = curAABB.max;
    n.centroid = curAABB.center();
    std::copy(children.cbegin(), children.cend(), std::begin(n.children));
    n.regionCount = 0;
    n.regionStart = 0;
  }
}

template void AROTBuilder::Node::pathWrite(DNAPATH::PATH<DNAMP1::PAKBridge>& path, const zeus::CAABox& curAABB);
template void AROTBuilder::Node::pathWrite(DNAPATH::PATH<DNAMP2::PAKBridge>& path, const zeus::CAABox& curAABB);
template void AROTBuilder::Node::pathWrite(DNAPATH::PATH<DNAMP3::PAKBridge>& path, const zeus::CAABox& curAABB);

void AROTBuilder::build(std::vector<std::vector<uint8_t>>& secs, const zeus::CAABox& fullAabb,
                        const std::vector<zeus::CAABox>& meshAabbs, const std::vector<DNACMDL::Mesh>& meshes) {
  /* Recursively split */
  BspNodeType rootType;
  rootNode.addChild(0, AROT_MIN_MODELS, meshAabbs, fullAabb, rootType);

  /* Calculate indexing metrics */
  size_t totalNodeCount = 0;
  size_t idxRefCount = 0;
  size_t curOff = 0;
  rootNode.nodeCount(totalNodeCount, idxRefCount, bmpPool, curOff);
  size_t bmpWordCount = ROUND_UP_32(meshes.size()) / 32;
  size_t arotSz = 64 + bmpWordCount * bmpPool.m_pool.size() * 4 + totalNodeCount * 8 + idxRefCount * 2;

  /* Write header */
  secs.emplace_back(arotSz, 0);
  athena::io::MemoryWriter w(secs.back().data(), secs.back().size());
  w.writeUint32Big('AROT');
  w.writeUint32Big(1);
  w.writeUint32Big(bmpPool.m_pool.size());
  w.writeUint32Big(meshes.size());
  w.writeUint32Big(totalNodeCount);
  w.writeVec3fBig(fullAabb.min);
  w.writeVec3fBig(fullAabb.max);
  w.seekAlign32();

  /* Write bitmap */
  std::vector<uint32_t> bmpWords;
  bmpWords.reserve(bmpWordCount);
  for (const std::set<int>& bmp : bmpPool.m_pool) {
    bmpWords.clear();
    bmpWords.resize(bmpWordCount);

    auto bmpIt = bmp.cbegin();
    if (bmpIt != bmp.cend()) {
      int curIdx = 0;
      for (size_t word = 0; word < bmpWordCount; ++word) {
        for (u32 b = 0; b < 32; ++b) {
          if (*bmpIt == curIdx) {
            bmpWords[word] |= 1U << b;
            ++bmpIt;
            if (bmpIt == bmp.cend()) {
              break;
            }
          }
          ++curIdx;
        }
        if (bmpIt == bmp.cend()) {
          break;
        }
      }
    }

    for (uint32_t word : bmpWords)
      w.writeUint32Big(word);
  }

  /* Write the rest */
  rootNode.writeIndirectionTable(w);
  rootNode.writeNodes(w, 0);
}

std::pair<std::unique_ptr<uint8_t[]>, uint32_t> AROTBuilder::buildCol(const ColMesh& mesh, BspNodeType& rootOut) {
  /* Accumulate total AABB */
  zeus::CAABox fullAABB;
  for (const auto& vert : mesh.verts)
    fullAABB.accumulateBounds(zeus::CVector3f(vert));

  /* Predetermine triangle AABBs */
  std::vector<zeus::CAABox> triBoxes;
  triBoxes.reserve(mesh.trianges.size());
  for (const ColMesh::Triangle& tri : mesh.trianges) {
    zeus::CAABox& aabb = triBoxes.emplace_back();
    for (const u32 edgeIdx : tri.edges) {
      const ColMesh::Edge& edge = mesh.edges[edgeIdx];
      for (const u32 vertIdx : edge.verts) {
        const auto& vert = mesh.verts[vertIdx];
        aabb.accumulateBounds(zeus::CVector3f(vert));
      }
    }
  }

  /* Recursively split */
  rootNode.addChild(0, COLLISION_MIN_NODE_TRIANGLES, triBoxes, fullAABB, rootOut);

  /* Calculate offsets and write out */
  size_t totalSize = 0;
  rootNode.colSize(totalSize);
  std::unique_ptr<uint8_t[]> ret(new uint8_t[totalSize]);
  uint8_t* ptr = ret.get();
  rootNode.writeColNodes(ptr, fullAABB);

  return {std::move(ret), totalSize};
}

template <class PAKBridge>
void AROTBuilder::buildPath(DNAPATH::PATH<PAKBridge>& path) {
  /* Accumulate total AABB and gather region boxes */
  std::vector<zeus::CAABox> regionBoxes;
  regionBoxes.reserve(path.regions.size());
  zeus::CAABox fullAABB;
  for (const auto& r : path.regions)
    fullAABB.accumulateBounds(regionBoxes.emplace_back(r.aabb[0], r.aabb[1]));

  /* Recursively split */
  BspNodeType dontCare;
  rootNode.addChild(0, PATH_MIN_NODE_REGIONS, regionBoxes, fullAABB, dontCare);

  /* Write out */
  size_t nodeCount = 0;
  size_t lookupCount = 0;
  rootNode.pathCountNodesAndLookups(nodeCount, lookupCount);
  path.octreeNodeCount = nodeCount;
  path.octree.reserve(nodeCount);
  path.octreeRegionLookupCount = lookupCount;
  path.octreeRegionLookup.reserve(lookupCount);
  rootNode.pathWrite(path, fullAABB);
}

template void AROTBuilder::buildPath(DNAPATH::PATH<DNAMP1::PAKBridge>& path);
template void AROTBuilder::buildPath(DNAPATH::PATH<DNAMP2::PAKBridge>& path);
template void AROTBuilder::buildPath(DNAPATH::PATH<DNAMP3::PAKBridge>& path);

} // namespace DataSpec
