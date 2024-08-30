#pragma once
#include "mc/world/ActorUniqueID.h"

namespace land {

using LandID  = uint64_t;    // 领地ID
using ChunkID = uint64_t;    // 区块ID
using LandDim = int;         // 领地所在维度
using UUIDm   = mce::UUID;   // class
using UUIDs   = std::string; // string

enum class LandPermType : int {
    Operator, // 领地操作员（管理）
    Owner,    // 领地主人
    Member,   // 领地成员
    Guest,    // 访客
};

} // namespace land