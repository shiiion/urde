set(COLLISION_SOURCES
    CAreaOctTree.hpp CAreaOctTree.cpp
    CollisionUtil.hpp CollisionUtil.cpp
    CGameCollision.hpp CGameCollision.cpp
    CCollisionResponseData.hpp CCollisionResponseData.cpp
    CCollisionInfo.hpp CCollisionInfo.cpp
    CCollisionInfoList.hpp
    CCollisionEdge.hpp CCollisionEdge.cpp
    CCollisionSurface.hpp CCollisionSurface.cpp
    InternalColliders.hpp InternalColliders.cpp
    CMetroidAreaCollider.hpp CMetroidAreaCollider.cpp
    COBBTree.hpp COBBTree.cpp
    CCollidableAABox.hpp CCollidableAABox.cpp
    CCollidableCollisionSurface.hpp CCollidableCollisionSurface.cpp
    CCollidableSphere.hpp CCollidableSphere.cpp
    CCollidableOBBTree.hpp CCollidableOBBTree.cpp
    CCollidableOBBTreeGroup.hpp CCollidableOBBTreeGroup.cpp
    CCollisionPrimitive.hpp CCollisionPrimitive.cpp
    CMaterialList.hpp
    CMaterialFilter.hpp CMaterialFilter.cpp
    CInternalRayCastStructure.hpp
    CRayCastResult.hpp CRayCastResult.cpp
    CCollisionActor.hpp CCollisionActor.cpp
    CCollisionActorManager.hpp CCollisionActorManager.cpp
    CJointCollisionDescription.hpp CJointCollisionDescription.cpp
    CAABoxFilter.hpp CAABoxFilter.cpp
    CBallFilter.hpp CBallFilter.cpp
    ICollisionFilter.hpp)

runtime_add_list(Collision COLLISION_SOURCES)
