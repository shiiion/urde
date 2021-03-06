#include "DataSpec/DNACommon/OBBTreeBuilder.hpp"

#include <cstddef>
#include <unordered_set>
#include <vector>

#include "DataSpec/DNAMP1/DCLN.hpp"

#include <athena/Types.hpp>
#include <gmm/gmm.h>
#include <hecl/Blender/Connection.hpp>
#include <zeus/CTransform.hpp>

namespace DataSpec {

using ColMesh = hecl::blender::ColMesh;

struct FittedOBB {
  zeus::CTransform xf;
  zeus::CVector3f he;
};

static std::vector<int> MakeRootTriangleIndex(const ColMesh& mesh) {
  std::vector<int> ret;
  ret.reserve(mesh.trianges.size());
  for (size_t i = 0; i < mesh.trianges.size(); ++i)
    ret.push_back(i);
  return ret;
}

static std::unordered_set<uint32_t> GetTriangleVerts(const ColMesh& mesh, int triIdx) {
  const ColMesh::Triangle& T = mesh.trianges[triIdx];
  std::unordered_set<uint32_t> verts;
  verts.insert(mesh.edges[T.edges[0]].verts[0]);
  verts.insert(mesh.edges[T.edges[0]].verts[1]);
  verts.insert(mesh.edges[T.edges[1]].verts[0]);
  verts.insert(mesh.edges[T.edges[1]].verts[1]);
  verts.insert(mesh.edges[T.edges[2]].verts[0]);
  verts.insert(mesh.edges[T.edges[2]].verts[1]);
  return verts;
}

// method to set the OBB parameters which produce a box oriented according to
// the covariance matrix C, which just containts the points pnts
static FittedOBB BuildFromCovarianceMatrix(gmm::dense_matrix<float>& C, const ColMesh& mesh,
                                           const std::vector<int>& index) {
  FittedOBB ret;

  // extract the eigenvalues and eigenvectors from C
  gmm::dense_matrix<float> eigvec(3, 3);
  std::vector<float> eigval(3);
  using namespace gmm;
  using MAT1 = gmm::dense_matrix<float>;
  gmm::symmetric_qr_algorithm(C, eigval, eigvec, default_tol_for_qr);

  // find the right, up and forward vectors from the eigenvectors
  zeus::CVector3f r(eigvec(0, 0), eigvec(1, 0), eigvec(2, 0));
  zeus::CVector3f f(eigvec(0, 1), eigvec(1, 1), eigvec(2, 1));
  zeus::CVector3f u(eigvec(0, 2), eigvec(1, 2), eigvec(2, 2));
  r.normalize();
  f.normalize();
  u.normalize();

  // set the rotation matrix using the eigvenvectors
  ret.xf.basis[0] = r;
  ret.xf.basis[1] = f;
  ret.xf.basis[2] = u;
  ret.xf.orthonormalize();

  // now build the bounding box extents in the rotated frame
  zeus::CVector3f minim(1e10f, 1e10f, 1e10f), maxim(-1e10f, -1e10f, -1e10f);
  for (int triIdx : index) {
    std::unordered_set<uint32_t> verts = GetTriangleVerts(mesh, triIdx);
    for (uint32_t v : verts) {
      const zeus::CVector3f& p = mesh.verts[v].val;
      zeus::CVector3f p_prime(ret.xf.basis[0].dot(p), ret.xf.basis[1].dot(p), ret.xf.basis[2].dot(p));
      minim = zeus::min(minim, p_prime);
      maxim = zeus::max(maxim, p_prime);
    }
  }

  // set the center of the OBB to be the average of the
  // minimum and maximum, and the extents be half of the
  // difference between the minimum and maximum
  zeus::CVector3f center = (maxim + minim) * 0.5f;
  ret.xf.origin = ret.xf.basis * center;
  ret.he = (maxim - minim) * 0.5f;

  return ret;
}

// builds an OBB from triangles specified as an array of
// points with integer indices into the point array. Forms
// the covariance matrix for the triangles, then uses the
// method build_from_covariance_matrix() method to fit
// the box.  ALL points will be fit in the box, regardless
// of whether they are indexed by a triangle or not.
static FittedOBB FitOBB(const ColMesh& mesh, const std::vector<int>& index) {
  float Ai, Am = 0.0;
  zeus::CVector3f mu, mui;
  gmm::dense_matrix<float> C(3, 3);
  float cxx = 0.0, cxy = 0.0, cxz = 0.0, cyy = 0.0, cyz = 0.0, czz = 0.0;

  // loop over the triangles this time to find the
  // mean location
  for (int i : index) {
    std::unordered_set<uint32_t> verts = GetTriangleVerts(mesh, i);
    auto it = verts.begin();
    zeus::CVector3f p = mesh.verts[*it++].val;
    zeus::CVector3f q = mesh.verts[*it++].val;
    zeus::CVector3f r = mesh.verts[*it++].val;
    mui = (p + q + r) / 3.f;
    Ai = (q - p).cross(r - p).magnitude() / 2.f;
    mu += mui * Ai;
    Am += Ai;

    // these bits set the c terms to Am*E[xx], Am*E[xy], Am*E[xz]....
    cxx += (9.0 * mui.x() * mui.x() + p.x() * p.x() + q.x() * q.x() + r.x() * r.x()) * (Ai / 12.0);
    cxy += (9.0 * mui.x() * mui.y() + p.x() * p.y() + q.x() * q.y() + r.x() * r.y()) * (Ai / 12.0);
    cxz += (9.0 * mui.x() * mui.z() + p.x() * p.z() + q.x() * q.z() + r.x() * r.z()) * (Ai / 12.0);
    cyy += (9.0 * mui.y() * mui.y() + p.y() * p.y() + q.y() * q.y() + r.y() * r.y()) * (Ai / 12.0);
    cyz += (9.0 * mui.y() * mui.z() + p.y() * p.z() + q.y() * q.z() + r.y() * r.z()) * (Ai / 12.0);
  }

  if (zeus::close_enough(Am, 0.f))
    return {};

  // divide out the Am fraction from the average position and
  // covariance terms
  mu = mu / Am;
  cxx /= Am;
  cxy /= Am;
  cxz /= Am;
  cyy /= Am;
  cyz /= Am;
  czz /= Am;

  // now subtract off the E[x]*E[x], E[x]*E[y], ... terms
  cxx -= mu.x() * mu.x();
  cxy -= mu.x() * mu.y();
  cxz -= mu.x() * mu.z();
  cyy -= mu.y() * mu.y();
  cyz -= mu.y() * mu.z();
  czz -= mu.z() * mu.z();

  // now build the covariance matrix
  C(0, 0) = cxx;
  C(0, 1) = cxy;
  C(0, 2) = cxz;
  C(1, 0) = cxy;
  C(1, 1) = cyy;
  C(1, 2) = cyz;
  C(2, 0) = cxz;
  C(2, 1) = cyz;
  C(2, 2) = czz;

  // set the obb parameters from the covariance matrix
  return BuildFromCovarianceMatrix(C, mesh, index);
}

template <typename Node>
static void MakeLeaf(const ColMesh& mesh, const std::vector<int>& index, Node& n) {
  n.left.reset();
  n.right.reset();
  n.isLeaf = true;
  n.leafData = std::make_unique<typename Node::LeafData>();
  n.leafData->triangleIndexCount = atUint32(index.size());
  n.leafData->triangleIndices.reserve(n.leafData->triangleIndexCount);
  for (int i : index)
    n.leafData->triangleIndices.push_back(i);
}

template <typename Node>
static std::unique_ptr<Node> RecursiveMakeNode(const ColMesh& mesh, const std::vector<int>& index) {
  // calculate root OBB
  FittedOBB obb = FitOBB(mesh, index);

  // make results row-major and also invert the rotation basis
  obb.xf.basis.transpose();

  std::unique_ptr<Node> n = std::make_unique<Node>();
  for (int i = 0; i < 3; ++i) {
    n->xf[i] = zeus::CVector4f{obb.xf.basis[i]};
    n->xf[i].simd[3] = float(obb.xf.origin[i]);
  }
  n->halfExtent = obb.he;

  // terminate branch when volume < 1.0
  if (obb.he[0] * obb.he[1] * obb.he[2] < 1.f) {
    MakeLeaf(mesh, index, *n);
    return n;
  }

  n->isLeaf = false;

  std::vector<int> indexNeg[3];
  std::vector<int> indexPos[3];
  for (int c = 0; c < 3; ++c) {
    // subdivide negative side
    indexNeg[c].reserve(index.size());
    for (int i : index) {
      std::unordered_set<uint32_t> verts = GetTriangleVerts(mesh, i);
      for (uint32_t vtx : verts) {
        zeus::CVector3f v = mesh.verts[vtx].val;
        v = obb.xf.basis * (v - obb.xf.origin);
        if (v[c] < 0.f) {
          indexNeg[c].push_back(i);
          break;
        }
      }
    }

    // subdivide positive side
    indexPos[c].reserve(index.size());
    for (int i : index) {
      std::unordered_set<uint32_t> verts = GetTriangleVerts(mesh, i);
      for (uint32_t vtx : verts) {
        zeus::CVector3f v = mesh.verts[vtx].val;
        v = obb.xf.basis * (v - obb.xf.origin);
        if (v[c] >= 0.f) {
          indexPos[c].push_back(i);
          break;
        }
      }
    }
  }

  size_t idxMin = index.size();
  int minComp = -1;
  for (int c = 0; c < 3; ++c) {
    size_t test = std::max(indexNeg[c].size(), indexPos[c].size());
    if (test < idxMin && test < index.size() * 3 / 4) {
      minComp = c;
      idxMin = test;
    }
  }

  if (minComp == -1) {
    MakeLeaf(mesh, index, *n);
    return n;
  }

  n->left = RecursiveMakeNode<Node>(mesh, indexNeg[minComp]);
  n->right = RecursiveMakeNode<Node>(mesh, indexPos[minComp]);

  return n;
}

template <typename Node>
std::unique_ptr<Node> OBBTreeBuilder::buildCol(const ColMesh& mesh) {
  std::vector<int> root = MakeRootTriangleIndex(mesh);
  return RecursiveMakeNode<Node>(mesh, root);
}

template std::unique_ptr<DNAMP1::DCLN::Collision::Node>
OBBTreeBuilder::buildCol<DNAMP1::DCLN::Collision::Node>(const ColMesh& mesh);

} // namespace DataSpec
