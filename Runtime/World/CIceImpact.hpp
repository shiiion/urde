#ifndef CICEIMPACT_HPP
#define CICEIMPACT_HPP

#include "CEffect.hpp"

namespace urde
{

class CIceImpact : public CEffect
{
public:
    CIceImpact(const TLockedToken<CGenDescription>& particle, TUniqueId uid, TAreaId aid,
               bool active, std::string_view name, const zeus::CTransform& xf, u32 flags,
               const zeus::CVector3f& scale, const zeus::CColor& color);
    void Accept(IVisitor& visitor);
};

}

#endif // CICEIMPACT_HPP
