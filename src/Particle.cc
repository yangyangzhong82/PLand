#include "pland/Particle.h"
#include "mc/network/packet/SpawnParticleEffectPacket.h"
#include <optional>

namespace land {


bool Particle::drawCube(const BlockPos& pos1, const BlockPos& pos2, int dim, std::string const& particle) {
    std::optional<class MolangVariableMap> molang;


    SpawnParticleEffectPacket pkt(pos1, particle, dim, molang);
    pkt.sendToClients();

    return true;
}


} // namespace pland