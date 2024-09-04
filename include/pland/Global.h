#pragma once
#include "ll/api/i18n/I18n.h"
#include "mc/world/ActorUniqueID.h"


namespace land {

using LandID    = uint64_t;    // 领地ID
using ChunkID   = uint64_t;    // 区块ID
using LandDimid = int;         // 领地所在维度
using UUIDm     = mce::UUID;   // class
using UUIDs     = std::string; // string

enum class LandPermType : int {
    Operator, // 领地操作员（管理）
    Owner,    // 领地主人
    Member,   // 领地成员
    Guest,    // 访客
};


// Using
using string = std::string;
using ll::i18n_literals::operator""_tr;

} // namespace land