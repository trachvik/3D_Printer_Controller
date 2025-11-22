#include <Adafruit_MCP23X17.h>

class MCP23017 : public Adafruit_MCP23X17
{
public:
    MCP23017();
    bool init();
};