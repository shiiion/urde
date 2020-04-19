#include "Runtime/MP1/World/CIceSheegoth.hpp"

#include "Runtime/CSimplePool.hpp"
#include "Runtime/CStateManager.hpp"
#include "Runtime/Character/CPASAnimParmData.hpp"
#include "Runtime/Collision/CCollisionActor.hpp"
#include "Runtime/Collision/CCollisionActorManager.hpp"
#include "Runtime/GameGlobalObjects.hpp"
#include "Runtime/Particle/CElementGen.hpp"
#include "Runtime/Particle/CParticleElectric.hpp"
#include "Runtime/Particle/CParticleSwoosh.hpp"
#include "Runtime/World/CGameArea.hpp"
#include "Runtime/World/CPatternedInfo.hpp"
#include "Runtime/World/CPlayer.hpp"
#include "Runtime/World/CWorld.hpp"

#include "TCastTo.hpp" // Generated file, do not modify include path

namespace urde::MP1 {
namespace {
constexpr std::array<SSphereJointInfo, 7> skSphereJointList = {{
    {"Jaw_end_LCTR", 0.55f},
    {"Pelvis", 1.1f},
    {"LCTR_SHEMOUTH", 1.1f},
    {"butt_LCTR", 0.7f},
    {"Ice_Shards_LCTR", 1.2f},
    {"GillL_LCTR", 0.6f},
    {"GillR_LCTR", 0.6f},
}};

constexpr std::array<SJointInfo, 2> skLeftLegJointList = {{
    {"L_hip", "L_knee", 0.4f, 0.75f},
    {"L_ankle", "L_Toe_3", 0.4f, 0.75f},
}};

constexpr std::array<SJointInfo, 2> skRightLegJointList = {{
    {"R_hip", "R_knee", 0.4f, 0.75f},
    {"R_ankle", "R_Toe_3", 0.4f, 0.75f},
}};
} // namespace

CIceSheegothData::CIceSheegothData(CInputStream& in, [[maybe_unused]] s32 propertyCount)
: x0_(zeus::degToRad(in.readFloatBig()))
, x4_(zeus::degToRad(in.readFloatBig()))
, x8_(zeus::CVector3f::ReadBig(in))
, x14_(in.readFloatBig())
, x18_(in)
, x80_(in)
, xe8_(in)
, x150_(in)
, x154_(in)
, x170_(in.readFloatBig())
, x174_(in.readFloatBig())
, x178_(in)
, x17c_(in)
, x180_(in)
, x19c_(in)
, x1a0_(in)
, x1a4_(in)
, x1a8_(in)
, x1ac_(in)
, x1b0_(in.readFloatBig())
, x1b4_(in.readFloatBig())
, x1b8_(in)
, x1d4_(CSfxManager::TranslateSFXID(in.readUint32Big()))
, x1d8_(in.readFloatBig())
, x1dc_(in.readFloatBig())
, x1e0_(in.readFloatBig())
, x1e4_(in)
, x1e8_(CSfxManager::TranslateSFXID(in.readUint32Big()))
, x1ec_(in)
, x1f0_24_(in.readBool())
, x1f0_25_(in.readBool()) {}

CIceSheegoth::CIceSheegoth(TUniqueId uid, std::string_view name, const CEntityInfo& info, zeus::CTransform& xf,
                           CModelData&& mData, const CPatternedInfo& pInfo, const CActorParameters& actParms,
                           const CIceSheegothData& sheegothData)
: CPatterned(ECharacter::IceSheeegoth, uid, name, EFlavorType::Zero, info, xf, std::move(mData), pInfo,
             EMovementType::Ground, EColliderType::One, EBodyType::BiPedal, actParms, EKnockBackVariant::Large)
, x56c_(sheegothData)
, x760_(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f)
, x844_(nullptr, 1, pInfo.GetPathfindingIndex(), 1.f, 1.f)
, x94c_(x3b4_speed)
, x974_(sheegothData.Get_x174())
, x98c_mouthVulnerability(pInfo.GetDamageVulnerability())
, x9f4_(*GetModelData()->GetAnimationData(), "Head_1"sv, zeus::degToRad(80.f), zeus::degToRad(180.f),
        EBoneTrackingFlags::None)
, xa30_(GetBoundingBox(), GetMaterialList())
, xa58_(sheegothData.Get_x150(), sheegothData.Get_x154())
, xa84_(sheegothData.Get_x178().IsValid() ? g_SimplePool->GetObj({SBIG('WPSC'), sheegothData.Get_x178()})
                                          : g_SimplePool->GetObj("FlameThrower"sv))
, xa8c_(g_SimplePool->GetObj({SBIG('PART'), sheegothData.Get_x1a0()}))
, xa9c_(std::make_unique<CElementGen>(xa8c_, CElementGen::EModelOrientationType::Normal,
                                      CElementGen::EOptionalSystemFlags::One))
, xaa0_(g_SimplePool->GetObj({SBIG('PART'), sheegothData.Get_x1a4()}))
, xab0_(std::make_unique<CElementGen>(xaa0_, CElementGen::EModelOrientationType::Normal,
                                      CElementGen::EOptionalSystemFlags::One))
, xab4_(g_SimplePool->GetObj({SBIG('PART'), sheegothData.Get_x1a4()}))
, xac4_(std::make_unique<CElementGen>(xaa0_, CElementGen::EModelOrientationType::Normal,
                                      CElementGen::EOptionalSystemFlags::One))
, xac8_(g_SimplePool->GetObj({SBIG('ELSC'), sheegothData.Get_x1ac()}))
, xad8_(std::make_unique<CParticleElectric>(xac8_))
, xb28_24_shotAt(false)
, xb28_25_(false)
, xb28_26_(false)
, xb28_27_(false)
, xb28_28_(false)
, xb28_29_(false)
, xb28_30_(false)
, xb28_31_(false)
, xb29_24_(false)
, xb29_25_(false)
, xb29_26_(false)
, xb29_27_(false)
, xb29_28_(false)
, xb29_29_(false) {

  UpdateTouchBounds();
  x460_knockBackController.SetEnableFreeze(false);
  x460_knockBackController.SetX82_24(false);
  x460_knockBackController.SetEnableLaggedBurnDeath(false);
  x460_knockBackController.SetEnableExplodeDeath(false);
  x950_ = GetAnimationDistance(CPASAnimParmData(3, CPASAnimParm::FromEnum(1), CPASAnimParm::FromEnum(0))) *
          GetModelData()->GetScale().y();
  xa9c_->SetGlobalScale(GetModelData()->GetScale());
  xab0_->SetGlobalScale(GetModelData()->GetScale());
  xac4_->SetGlobalScale(GetModelData()->GetScale());
  xad8_->SetGlobalScale(GetModelData()->GetScale());
  x450_bodyController->BodyStateInfo().SetLocoAnimChangeAtEndOfAnimOnly(true);
  MakeThermalColdAndHot();
  x328_31_energyAttractor = true;
  // TODO: Remove
  CPatterned::SetActive(true);
}
void CIceSheegoth::Accept(IVisitor& visitor) { visitor.Visit(this); }
void CIceSheegoth::Think(float dt, CStateManager& mgr) { CPatterned::Think(dt, mgr); }
void CIceSheegoth::AcceptScriptMsg(EScriptObjectMessage msg, TUniqueId sender, CStateManager& mgr) {
  switch (msg) {
  case EScriptObjectMessage::Activate: {
    xa2c_collisionManager->SetActive(mgr, true);
    break;
  }
  case EScriptObjectMessage::Deactivate: {
    xa2c_collisionManager->SetActive(mgr, false);
    break;
  }
  case EScriptObjectMessage::Start: {
    xb28_24_shotAt = true;
    break;
  }
  case EScriptObjectMessage::Touched: {
    ApplyContactDamage(sender, mgr);
    if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(sender)) {
      if (const TCastToConstPtr<CWeapon> wp = mgr.GetObjectById(colAct->GetLastTouchedObject())) {
        if (wp->GetOwnerId() == mgr.GetPlayer().GetUniqueId()) {
          xb28_24_shotAt = true;
        }
      }
    }
    break;
  }
  case EScriptObjectMessage::Registered: {
    if (!HasPatrolPath(mgr, 0.f)) {
      x450_bodyController->SetLocomotionType(pas::ELocomotionType::Crouch);
    }
    x450_bodyController->Activate(mgr);
    SetupCollisionActorManager(mgr);
    CreateFlameThrower(mgr);
    if (x450_bodyController->GetBodyStateInfo().GetMaxSpeed() > 0.f) {
      x944_ = x948_ = (0.9f * x450_bodyController->GetBodyStateInfo().GetLocomotionSpeed(pas::ELocomotionAnim::Walk)) /
                      x450_bodyController->GetBodyStateInfo().GetMaxSpeed();
    }
    GetBodyController()->GetCommandMgr().SetSteeringBlendMode(ESteeringBlendMode::FullSpeed);
    xaf4_mouthLocator = GetModelData()->GetAnimationData()->GetLocatorSegId("LCTR_SHEMOUTH"sv);
    break;
  }
  case EScriptObjectMessage::Deleted: {
    xa2c_collisionManager->Destroy(mgr);
    if (xa80_ != kInvalidUniqueId) {
      mgr.FreeScriptObject(xa80_);
      xa80_ = kInvalidUniqueId;
    }
    if (xaf0_) {
      CSfxManager::RemoveEmitter(xaf0_);
      xaf0_.reset();
    }
    break;
  }
  case EScriptObjectMessage::InitializedInArea: {
    x760_.SetArea(mgr.GetWorld()->GetArea(GetAreaIdAlways())->GetPostConstructed()->x10bc_pathArea);
    x844_.SetArea(mgr.GetWorld()->GetArea(GetAreaIdAlways())->GetPostConstructed()->x10bc_pathArea);
    mgr.SetBossParams(GetUniqueId(), GetHealthInfo(mgr)->GetHP(), 0);
    break;
  }
  case EScriptObjectMessage::Damage: {
    if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(sender)) {
      if (const TCastToConstPtr<CWeapon> wp = mgr.GetObjectById(colAct->GetLastTouchedObject())) {
        if (sender == xaf6_ && !xb28_27_) {
          sub_8019ebf0(mgr, wp->GetDamageInfo().GetDamage());
          if (!xaec_ || xaec_->IsSystemDeletable()) {
            xaec_ = std::make_unique<CElementGen>(xadc_, CElementGen::EModelOrientationType::Normal,
                                                  CElementGen::EOptionalSystemFlags::One);
          }
        } else {
          TakeDamage(zeus::skZero3f, 0.f);
          if (sub_8019fc40(colAct)) {
            x97c_ = 0.2f;
            x980_ = GetTransform().basis[1];
          }
        }
      }
    } else {
      ApplyWeaponDamage(mgr, sender);
    }
    xb28_24_shotAt = true;
    x968_ = 0.f;
    mgr.InformListeners(GetTranslation(), EListenNoiseType::PlayerFire);
    break;
  }
  case EScriptObjectMessage::InvulnDamage: {
    if (sender == xaf6_ && !xb28_27_) {
      if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(sender)) {
        if (const TCastToConstPtr<CWeapon> wp = mgr.GetObjectById(colAct->GetLastTouchedObject())) {
          sub_8019ebf0(mgr, wp->GetDamageInfo().GetDamage());
          if (!xaec_ || xaec_->IsSystemDeletable()) {
            xaec_ = std::make_unique<CElementGen>(xadc_, CElementGen::EModelOrientationType::Normal,
                                                  CElementGen::EOptionalSystemFlags::One);
          }
        }
      }
    }
    mgr.InformListeners(GetTranslation(), EListenNoiseType::PlayerFire);
    xb28_24_shotAt = true;
    x968_ = 0.f;
    break;
  }
  case EScriptObjectMessage::SuspendedMove: {
    if (xa2c_collisionManager != nullptr) {
      xa2c_collisionManager->SetMovable(mgr, false);
    }
    break;
  }
  default:
    break;
  }
  CPatterned::AcceptScriptMsg(msg, sender, mgr);
}
void CIceSheegoth::AddToRenderer(const zeus::CFrustum& frustum, CStateManager& mgr) {
  CPatterned::AddToRenderer(frustum, mgr);
}
void CIceSheegoth::Render(CStateManager& mgr) { CPatterned::Render(mgr); }
zeus::CVector3f CIceSheegoth::GetAimPosition(const CStateManager& mgr, float dt) const {
  return CPatterned::GetAimPosition(mgr, dt);
}
EWeaponCollisionResponseTypes CIceSheegoth::GetCollisionResponseType(const zeus::CVector3f& v1,
                                                                     const zeus::CVector3f& v2, const CWeaponMode& mode,
                                                                     EProjectileAttrib attrib) const {
  return mode.GetType() == EWeaponType::Ice ? EWeaponCollisionResponseTypes::None
                                            : CPatterned::GetCollisionResponseType(v1, v2, mode, attrib);
}
zeus::CAABox CIceSheegoth::GetSortingBounds(const CStateManager& mgr) const { return CActor::GetSortingBounds(mgr); }
void CIceSheegoth::DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type, float dt) {
  CPatterned::DoUserAnimEvent(mgr, node, type, dt);
}
void CIceSheegoth::Patrol(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::PathFind(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::TargetPlayer(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Generate(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Deactivate(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Attack(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::DoubleSnap(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::TurnAround(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Crouch(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Taunt(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::ProjectileAttack(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Flinch(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Approach(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::Enraged(CStateManager& mgr, EStateMsg msg, float dt) {}
void CIceSheegoth::SpecialAttack(CStateManager& mgr, EStateMsg msg, float dt) {}
bool CIceSheegoth::Leash(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::OffLine(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::TooClose(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::InMaxRange(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::InDetectionRange(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::SpotPlayer(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::AnimOver(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::AggressionCheck(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::ShouldFire(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::ShouldFlinch(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::ShotAt(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::ShouldSpecialAttack(CStateManager& mgr, float arg) { return false; }
bool CIceSheegoth::LostInterest(CStateManager& mgr, float arg) { return false; }
void CIceSheegoth::UpdateTouchBounds() {
  x978_ = 1.75f * GetModelData()->GetScale().y();
  zeus::CAABox box{-x978_, -x978_, 0.f, x978_, x978_, 2.f * x978_};
  SetBoundingBox(box);
  xa30_.SetBox(box);
  x760_.SetCharacterRadius(x978_);
  x760_.SetCharacterHeight(x978_);
  x760_.SetPadding(20.f);
  x844_.SetCharacterRadius(x978_);
  x844_.SetCharacterHeight(x978_);
  x844_.SetPadding(20.f);
}
void CIceSheegoth::ApplyContactDamage(TUniqueId sender, CStateManager& mgr) {
  if (const TCastToConstPtr<CCollisionActor> colAct = mgr.GetObjectById(sender)) {
    if (colAct->GetHealthInfo(mgr)->GetHP() <= 0.f)
      return;
    bool bVar5 = (xb28_29_ && !xb28_25_ && xb28_28_) ? true : sub_8019fc84(sender);

    if (colAct->GetLastTouchedObject() == mgr.GetPlayer().GetUniqueId()) {
      if (bVar5) {
        mgr.ApplyDamage(GetUniqueId(), mgr.GetPlayer().GetUniqueId(), GetUniqueId(), x56c_.Get_x1b8(),
                        CMaterialFilter::MakeIncludeExclude({EMaterialTypes::Solid}, {}), zeus::skZero3f);
      } else {
        mgr.ApplyDamage(GetUniqueId(), mgr.GetPlayer().GetUniqueId(), GetUniqueId(), GetContactDamage(),
                        CMaterialFilter::MakeIncludeExclude({EMaterialTypes::Solid}, {}), zeus::skZero3f);
      }
    }
  }
}
void CIceSheegoth::AddSphereCollisionList(const SSphereJointInfo* info, size_t count,
                                          std::vector<CJointCollisionDescription>& vecOut) {
  for (size_t i = 0; i < count; ++i) {
    const auto& joint = info[i];
    CSegId id = GetModelData()->GetAnimationData()->GetLocatorSegId(joint.name);
    if (id.IsInvalid()) {
      continue;
    }
    vecOut.push_back(CJointCollisionDescription::SphereCollision(id, joint.radius, joint.name, 1000.f));
  }
}
void CIceSheegoth::AddCollisionList(const SJointInfo* info, size_t count,
                                    std::vector<CJointCollisionDescription>& vecOut) {
  for (size_t i = 0; i < count; ++i) {
    const auto& joint = info[i];
    CSegId fromId = GetModelData()->GetAnimationData()->GetLocatorSegId(joint.from);
    CSegId toId = GetModelData()->GetAnimationData()->GetLocatorSegId(joint.to);
    if (fromId.IsInvalid() || toId.IsInvalid()) {
      continue;
    }
    vecOut.push_back(CJointCollisionDescription::SphereSubdivideCollision(
        fromId, toId, joint.radius, joint.separation, CJointCollisionDescription::EOrientationType::One, joint.from,
        1000.f));
  }
}
void CIceSheegoth::SetupCollisionActorManager(CStateManager& mgr) {
  std::vector<CJointCollisionDescription> joints;
  joints.reserve(7);
  AddSphereCollisionList(skSphereJointList.data(), skSphereJointList.size(), joints);
  AddCollisionList(skLeftLegJointList.data(), skLeftLegJointList.size(), joints);
  AddCollisionList(skRightLegJointList.data(), skRightLegJointList.size(), joints);
  xa2c_collisionManager = std::make_unique<CCollisionActorManager>(mgr, GetUniqueId(), GetAreaIdAlways(), joints, true);
  xa2c_collisionManager->SetActive(mgr, GetActive());
  xa2c_collisionManager->AddMaterial(mgr, EMaterialTypes::CameraPassthrough);
  xb04_.clear();
  xafc_.clear();

  for (size_t i = 0; i < xa2c_collisionManager->GetNumCollisionActors(); ++i) {
    const auto& desc = xa2c_collisionManager->GetCollisionDescFromIndex(i);
    if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(desc.GetCollisionActorId())) {
      colAct->SetDamageVulnerability(CDamageVulnerability::ImmuneVulnerabilty());
      if (desc.GetName().find("LCTR_SHEMOUTH"sv) != std::string_view::npos) {
        xaf8_mouthCollider = desc.GetCollisionActorId();
        colAct->SetDamageVulnerability(x98c_mouthVulnerability);
      } else if (desc.GetName().find("Jaw_end_LCTR"sv) != std::string_view::npos) {
        colAct->SetDamageVulnerability(CDamageVulnerability::PassThroughVulnerabilty());
      } else if (desc.GetName().find("Ice_Shards_LCTR"sv) != std::string_view::npos) {
        colAct->SetWeaponCollisionResponseType(EWeaponCollisionResponseTypes::None);
      } else if (desc.GetName().find("GillL_LCTR"sv) != std::string_view::npos ||
                 desc.GetName().find("GillR_LCTR"sv) != std::string_view::npos) {
        xafc_.emplace_back(desc.GetCollisionActorId());
      } else {
        xb04_.emplace_back(desc.GetCollisionActorId());
        colAct->SetDamageVulnerability(x56c_.Get_x80());
      }
    }
  }

  SetupHealthInfo(mgr);
  SetMaterialFilter(CMaterialFilter::MakeIncludeExclude(
      {EMaterialTypes::Solid},
      {EMaterialTypes::Player, EMaterialTypes::CollisionActor, EMaterialTypes::AIPassthrough}));
  AddMaterial(EMaterialTypes::ProjectilePassthrough, mgr);
  xa2c_collisionManager->AddMaterial(mgr, {EMaterialTypes::AIJoint, EMaterialTypes::CameraPassthrough});
}
void CIceSheegoth::SetupHealthInfo(CStateManager& mgr) {
  CHealthInfo* thisHealth = HealthInfo(mgr);
  for (auto& id : xafc_) {
    if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(id)) {
      *colAct->HealthInfo(mgr) = *thisHealth;
    }
  }

  if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(xaf8_mouthCollider)) {
    *colAct->HealthInfo(mgr) = *thisHealth;
  }

  for (auto& id : xb04_) {
    if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(id)) {
      *colAct->HealthInfo(mgr) = *thisHealth;
    }
  }
}
void CIceSheegoth::sub_8019ebf0(CStateManager& mgr, float damage) {
  x974_ = zeus::clamp(0.f, x974_ + damage, x56c_.Get_x170());
  float fVar1 = (1.f/3.f) * x56c_.Get_x170();
  if (fVar1 <= 0.f) {
    xb29_26_ = true;
  } else {
    xb29_26_ = mgr.GetActiveRandom()->Float() <= (x974_ / (3.f * fVar1));
  }
}

void CIceSheegoth::ApplyWeaponDamage(CStateManager& mgr, TUniqueId sender) {
  if (const TCastToConstPtr<CWeapon> wp = mgr.GetObjectById(sender)) {
    mgr.ApplyDamage(sender, xaf8_mouthCollider, wp->GetOwnerId(), wp->GetDamageInfo(),
                    CMaterialFilter::MakeIncludeExclude({EMaterialTypes::Solid}, {}), zeus::skZero3f);
  }
}
void CIceSheegoth::CreateFlameThrower(CStateManager& mgr) {}
} // namespace urde::MP1