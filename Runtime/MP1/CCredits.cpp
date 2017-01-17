#include "CCredits.hpp"

namespace urde
{
namespace MP1
{

CCredits::CCredits()
: CIOWin("Credits")
{

}

CIOWin::EMessageReturn CCredits::OnMessage(const CArchitectureMessage& msg, CArchitectureQueue& queue)
{
    return EMessageReturn::Normal;
}

void CCredits::Draw() const
{

}

}
}
