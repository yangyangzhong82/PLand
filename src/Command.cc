#include "pland/Command.h"
#include "pland/Particle.h"
#include "pland/utils/MC.h"


#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/service/Bedrock.h"
#include "mc/common/wrapper/optional_ref.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/network/packet/SetTimePacket.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/ChunkBlockPos.h"
#include "mc/world/level/ChunkPos.h"
#include "mc/world/level/Command.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/dimension/Dimension.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <ll/api/Logger.h>
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/i18n/I18n.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/service/PlayerInfo.h>
#include <ll/api/service/Service.h>
#include <ll/api/utils/HashUtils.h>
#include <map>
#include <mc/entity/utilities/ActorType.h>
#include <mc/enums/GameType.h>
#include <mc/network/packet/LevelChunkPacket.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/ServerLevel.h>
#include <mc/server/ServerPlayer.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOriginType.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandParameterOption.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/server/commands/CommandRegistry.h>
#include <mc/server/commands/CommandSelector.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>

using ll::command::CommandRegistrar;

namespace land {

bool LandCommand::setup() {
    auto& cmd = ll::command::CommandRegistrar::getInstance().getOrCreateCommand("pland", "PLand System");

    // pland selector start
    cmd.overload().text("selector").text("start").execute([](CommandOrigin const& ori, CommandOutput& out) {
        if (ori.getOriginType() != CommandOriginType::Player) {
            mc::sendText(out, "Only players can use this command");
            return;
        }

        auto& player = *static_cast<Player*>(ori.getEntity());
        bool  ok     = LandSelector::getInstance().tryStartSelect(player, player.getDimensionId(), true);
        if (ok) {
            mc::sendText(out, "Selecting land");
        } else {
            mc::sendText<mc::LogLevel::Error>(out, "You are already selecting a land");
        }
    });

    // pland selector pos1
    cmd.overload().text("selector").text("pos1").execute([](CommandOrigin const& ori, CommandOutput& out) {
        if (ori.getOriginType() != CommandOriginType::Player) {
            mc::sendText(out, "Only players can use this command");
            return;
        }

        auto& player = *static_cast<Player*>(ori.getEntity());
        bool  ok     = LandSelector::getInstance().trySelectPoint1(player, player.getPosition());
        if (ok) {
            mc::sendText(out, "Selected pos1");
        } else {
            mc::sendText<mc::LogLevel::Error>(out, "You are not selecting a land");
        }
    });

    // pland selector pos2
    cmd.overload().text("selector").text("pos2").execute([](CommandOrigin const& ori, CommandOutput& out) {
        if (ori.getOriginType() != CommandOriginType::Player) {
            mc::sendText(out, "Only players can use this command");
            return;
        }

        auto& player = *static_cast<Player*>(ori.getEntity());
        bool  ok     = LandSelector::getInstance().trySelectPoint2(player, player.getPosition());
        if (ok) {
            mc::sendText(out, "Selected pos2");
        } else {
            mc::sendText<mc::LogLevel::Error>(out, "You are not selecting a land");
        }
    });

    // pland selector stop
    cmd.overload().text("selector").text("stop").execute([](CommandOrigin const& ori, CommandOutput& out) {
        if (ori.getOriginType() != CommandOriginType::Player) {
            mc::sendText(out, "Only players can use this command");
            return;
        }

        auto& player = *static_cast<Player*>(ori.getEntity());
        bool  ok     = LandSelector::getInstance().tryStopSelect(player);
        if (ok) {
            mc::sendText(out, "Land selection ended");
        } else {
            mc::sendText<mc::LogLevel::Error>(out, "You are not selecting a land");
        }
    });
    return true;
}


} // namespace land