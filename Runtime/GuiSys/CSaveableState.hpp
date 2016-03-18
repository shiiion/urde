#ifndef __URDE_CSAVEABLESTATE_HPP__
#define __URDE_CSAVEABLESTATE_HPP__

#include "CDrawStringOptions.hpp"
#include "CToken.hpp"
#include "zeus/CColor.hpp"

namespace urde
{
class CRasterFont;

class CSaveableState
{
    CDrawStringOptions x0_drawStrOpts;
    TToken<CRasterFont> x14_token;
    std::vector<zeus::CColor> x20_;
    std::vector<u8> x30_;
    float x40_ = 1.f;
    u32 x44_ = 0;
    bool x48_ = false;
    u32 x4c_ = 0;
    u32 x50_ = 0;

    CSaveableState()
    {
        x20_.resize(3, zeus::CColor::skBlack);
        x30_.resize(16);
    }
};

}

#endif // __URDE_CSAVEABLESTATE_HPP__