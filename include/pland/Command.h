

namespace land {

class LandCommand {
public:
    LandCommand()                              = delete;
    LandCommand(const LandCommand&)            = delete;
    LandCommand& operator=(const LandCommand&) = delete;
    ~LandCommand()                             = delete;

    static bool setup();
};


} // namespace land