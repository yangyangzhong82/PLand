#include "mc/math/Vec3.h"
#include "mc/world/level/BlockPos.h"

namespace land {

class Particle {
public:
    Particle()                           = delete;
    Particle(Particle&&)                 = delete;
    Particle(const Particle&)            = delete;
    Particle& operator=(Particle&&)      = delete;
    Particle& operator=(const Particle&) = delete;
    ~Particle()                          = delete;

    static bool drawCube(
        BlockPos const&    pos1,
        BlockPos const&    pos2,
        int                dim,
        std::string const& particle = "minecraft:villager_happy"
    );
};


} // namespace pland
