#pragma once
#include "pland/Global.h"


namespace land {


class EventListener {
public:
    EventListener()                                = delete;
    EventListener(const EventListener&)            = delete;
    EventListener& operator=(const EventListener&) = delete;

    static bool setup();

    static bool release();

    static std::unordered_set<string> UseItemOnMap;
    static std::unordered_set<string> InteractBlockMap;
    static std::unordered_set<string> AnimalEntityMap;
    static std::unordered_set<string> MobEntityMap;
};


} // namespace land