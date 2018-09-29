
#include "Effects.h"

Effects::Effects() {
    currentCommand = &Effects::cmdEmpty;
    currentEffect  = &Effects::cmdEmpty;
}

void Effects::setFPS(uint8_t f)
{
    FPS = f;
}

void Effects::setLightStateController(LightStateController *l)
{
    lightState = l;
}

void Effects::setCommandFrames(uint16_t i)
{
    commandFrames = i;
}

void Effects::setCurrentCommand(Command cmd)
{
    LightState state = lightState->getCurrentState();
    setCommandFrames(FPS * (state.transition || 1));
    commandFrameCount = 0;
    commandStart = millis();

    switch(cmd) {
        case Command::Brightness :
            currentCommand = &Effects::cmdSetBrightness;
            break;
        default :
            currentCommand = &Effects::cmdEmpty;
    }
}

void Effects::runCurrentCommand()
{
    (this->*currentCommand)();
}

void Effects::cmdEmpty()
{

}

void Effects::cmdSetBrightness()
{
    LightState state = lightState->getCurrentState();

    uint8_t  target   = state.brightness;
    uint8_t  current  = FastLED.getBrightness();

    if (commandFrameCount < commandFrames) {
        FastLED.setBrightness(current + ((target - current) / (commandFrames - commandFrameCount)));
        commandFrameCount++;
    } else {
        Serial.printf(
            "  - command: setting brightness DONE [%i] %lu ms.\n",
            FastLED.getBrightness(), (millis() - commandStart)
        );

        setCurrentCommand(Command::None);
    }
}
