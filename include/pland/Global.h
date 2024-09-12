#pragma once
#include "ll/api/i18n/I18n.h"
#include "ll/api/schedule/Scheduler.h"
#include "mc/deps/core/mce/UUID.h"
#include "mc/world/ActorUniqueID.h"


namespace land {

// 全局类型定义
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
using ll::chrono_literals::operator""_tick; // 1s = 20_tick


// 全局共享资源 (这些对象在 Mod 初始化时创建，在 Mod 退出时销毁)
extern ll::schedule::GameTickScheduler GlobalTickScheduler; // 全局定时器


} // namespace land


#ifdef LDAPI_EXPORT
#define LDAPI __declspec(dllexport)
#else
#define LDAPI __declspec(dllimport)
#endif
