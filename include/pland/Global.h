#pragma once
#include "ll/api/chrono/GameChrono.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/i18n/I18n.h"
#include "mc/common/ActorUniqueID.h"
#include "mc/platform/UUID.h"
#include <filesystem>


namespace land {

// 全局类型定义
using LandID    = uint64_t;    // 领地ID
using ChunkID   = uint64_t;    // 区块ID
using LandDimid = int;         // 领地所在维度
using UUIDm     = mce::UUID;   // class
using UUIDs     = std::string; // string


enum class LandPermType : int {
    Operator = 0, // 领地操作员（管理）
    Owner,        // 领地主人
    Member,       // 领地成员
    Guest,        // 访客
};


// Using
using string = std::string;
using ll::i18n_literals::operator""_tr;
using ll::chrono_literals::operator""_tick; // 1s = 20_tick
namespace fs = std::filesystem;


} // namespace land


#ifdef LDAPI_EXPORT
#define LDAPI __declspec(dllexport)
#else
#define LDAPI __declspec(dllimport)
#endif
