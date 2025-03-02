#pragma once
#include "pland/Global.h"
#include <unordered_set>


namespace land {


class EventListener {
public:
    EventListener()                                = delete;
    EventListener(const EventListener&)            = delete;
    EventListener& operator=(const EventListener&) = delete;

    LDAPI static bool setup();

    LDAPI static bool release();

    LDAPI static std::unordered_set<string> InteractItemHashMap;
    LDAPI static std::unordered_set<string> InteractBlockHashMap;
};


} // namespace land