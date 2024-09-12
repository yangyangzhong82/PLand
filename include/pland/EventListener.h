#pragma once
#include "pland/Global.h"


namespace land {


class EventListener {
public:
    EventListener()                                = delete;
    EventListener(const EventListener&)            = delete;
    EventListener& operator=(const EventListener&) = delete;

    LDAPI static bool setup();

    LDAPI static bool release();

    LDAPI static std::unordered_set<string> UseItemOnMap;
    LDAPI static std::unordered_set<string> InteractBlockMap;
    LDAPI static std::unordered_set<string> AnimalEntityMap;
    LDAPI static std::unordered_set<string> MobEntityMap;
};


} // namespace land