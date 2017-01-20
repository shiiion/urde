#include "CAudioSys.hpp"
#include "CSimplePool.hpp"
#include "CAudioGroupSet.hpp"

namespace urde
{

CAudioSys* CAudioSys::g_SharedSys = nullptr;
static std::unordered_map<std::string, TLockedToken<CAudioGroupSet>> mpGroupSetDB;
static std::unordered_map<ResId, std::string> mpGroupSetResNameDB;
static const std::string mpDefaultInvalidString = "NULL";

TLockedToken<CAudioGroupSet> CAudioSys::FindGroupSet(const std::string& name)
{
    auto search = mpGroupSetDB.find(name);
    if (search == mpGroupSetDB.cend())
        return {};
    return search->second;
}

const std::string& CAudioSys::SysGetGroupSetName(ResId id)
{
    auto search = mpGroupSetResNameDB.find(id);
    if (search == mpGroupSetResNameDB.cend())
        return mpDefaultInvalidString;
    return search->second;
}

bool CAudioSys::SysLoadGroupSet(CSimplePool* pool, ResId id)
{
    if (!FindGroupSet(SysGetGroupSetName(id)))
    {
        TLockedToken<CAudioGroupSet> set = pool->GetObj(SObjectTag{FOURCC('AGSC'), id});
        mpGroupSetDB.emplace(std::make_pair(set->GetName(), set));
        mpGroupSetResNameDB.emplace(std::make_pair(id, set->GetName()));
        return false;
    }
    else
    {
        return true;
    }
}

bool CAudioSys::SysLoadGroupSet(const TLockedToken<CAudioGroupSet>& set, const std::string& name, ResId id)
{
    if (!FindGroupSet(name))
    {
        mpGroupSetDB.emplace(std::make_pair(set->GetName(), set));
        mpGroupSetResNameDB.emplace(std::make_pair(id, set->GetName()));
        return false;
    }
    else
    {
        return true;
    }
}

void CAudioSys::SysUnloadAudioGroupSet(const std::string& name)
{
    auto set = FindGroupSet(name);
    if (!set)
        return;

    mpGroupSetResNameDB.erase(set.GetObjectTag()->id);
    mpGroupSetDB.erase(name);
}

bool CAudioSys::SysIsGroupSetLoaded(const std::string& name)
{
    return FindGroupSet(name).operator bool();
}

void CAudioSys::SysAddGroupIntoAmuse(const std::string& name)
{
    if (auto set = FindGroupSet(name))
        AddAudioGroup(set->GetAudioGroupData());
}

void CAudioSys::SysRemoveGroupFromAmuse(const std::string& name)
{
    if (auto set = FindGroupSet(name))
        RemoveAudioGroup(set->GetAudioGroupData());
}

}
