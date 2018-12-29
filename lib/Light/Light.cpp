#include "Light.h"

Light::Light()
{
}

Light::Light(CRGB* l)
{
    addLeds(l);
}

Light::Light(CRGB* l, uint8_t fps, LightConfig lc, String username) {
    leds = l;
    FPS  = fps;
    name = lc.name;

    command_topic = String("/" + username + lc.command_topic);
    state_topic   = String("/" + username + lc.state_topic);
    filename      = String("/" + name + "_state.json");

    addSegment(Segment::Top, lc.top[0], lc.top[1]);
    addSegment(Segment::Bottom, lc.bottom[0], lc.bottom[1]);

    state.initialize(filename);

    effects = Effects(&state, FPS);
    effects.setTop(topStart, topStop);
    effects.setBottom(bottomStart, bottomStop);
}

Light& Light::addLeds(CRGB* l)
{
    leds = l;

    return *this;
}

Light& Light::addSegment(Segment segment, uint16_t start, uint16_t stop)
{
    switch (segment) {
        case Segment::Top :
            topStart = start;
            topStop  = stop;
            break;
        case Segment::Bottom :
            bottomStart = start;
            bottomStop  = stop;
            break;
        default:
            break;
    }

    return *this;
}

Light& Light::setCommandTopic(const char* topic) {
    command_topic = topic;
    return *this;
}

Light& Light::setStateTopic(const char* topic) {
    state_topic = topic;
    return *this;
}

String Light::getStateTopic()
{
    return state_topic;
}

String Light::getCommandTopic()
{
    return command_topic;
}

uint8_t Light::getBrightness() {
    return state.getCurrentState().brightness;
}

String Light::getStateAsJSON()
{
    char json[256];
    state.printStateJsonTo(json);
    return String(json);
}

String Light::getName()
{
    return name;
}

void Light::onCommand(byte* message)
{
    Serial.printf("%s: Got command\n", name.c_str());
    state.parseNewState(message);
    handleNewState();
}

void Light::handleNewState()
{
    LightStateData sd = state.getCurrentState();

    if (sd.state == false) {
        effects.setCurrentCommand(Effects::Command::TurnOff);
        return;
    }

    if (sd.state == true && sd.status.stateHasChanged) {
        effects.setCurrentCommand(Effects::Command::TurnOn);
        return;
    }

    if (sd.status.hasBrightness) handleBrightness(sd.brightness);
    if (sd.status.hasColor)      handleColor(sd.color);
    if (sd.status.hasEffect)     handleEffect(sd.effect);
    if (sd.status.hasColorTemp)  handleColorTemp(sd.color_temp);
}

uint8_t Light::getColorTempRed(uint16_t ct)
{
    unsigned int kelvin = (1000000/ct);
    unsigned int temp = kelvin / 100;

    double red = 0;
    if (temp <= 66) {
         red = 255;
    }
    else {
        red = temp - 60;
        red = 329.698727446 * (pow(red, -0.1332047592));
        if (red < 0)    red = 0;
        if (red > 255)  red = 255;
    }

    return static_cast<uint8_t>(red);
}

uint8_t Light::getColorTempGreen(uint16_t ct)
{
    unsigned int kelvin = (1000000/ct);
    unsigned int temp = kelvin / 100;

    double green = 0;
    if (temp <= 66) {
        green = temp;
        green = 99.4708025861 * log(green) - 161.1195681661;
        if (green < 0)      green = 0;
        if (green > 255)    green = 255;
    }
    else {
        green = temp - 60;
        green = 288.1221695283 * (pow(green, -0.0755148492));
        if (green < 0)      green = 0;
        if (green > 255)    green = 255;
    }

    return static_cast<uint8_t>(green);
}

uint8_t Light::getColorTempBlue(uint16_t ct)
{
    unsigned int kelvin = (1000000/ct);
    unsigned int temp = kelvin / 100;

    double blue = 0;
    if (temp >= 66) {
        blue = 255;
    }
    else {
        if (temp <= 19) {
            blue = 0;
        }
        else {
            blue = temp - 10;
            blue = 138.5177312231 * log(blue) - 305.0447927307;
            if (blue < 0)   blue = 0;
            if (blue > 255) blue = 255;
        }
    }

    return static_cast<uint8_t>(blue);
}

void Light::handleColorTemp(uint16_t colorTemp)
{
    Serial.printf("  - Got color temp: %i mired\n", colorTemp);

    Color c = {
        getColorTempRed(colorTemp),
        getColorTempGreen(colorTemp),
        getColorTempBlue(colorTemp)
    };
    state.setColor(c);

    effects.setCurrentCommand(Effects::Command::Color);
}

void Light::handleBrightness(uint8_t brightness)
{
    Serial.printf("  - Got new brightness: '%i'\n", brightness);
    if (isMaster()) {
        effects.setCurrentCommand(Effects::Command::GlobalBrightness);
    }
    else {
        effects.setCurrentCommand(Effects::Command::Brightness);
    }
}

void Light::handleEffect(String effect)
{
    Serial.printf("  - Got effect '%s'. Setting it.\n", effect.c_str());
    effects.setCurrentEffect(effect);

    if (effect.equals("")) effects.setCurrentCommand(Effects::Command::Color);
}

void Light::handleColor(Color color)
{
    Serial.printf("  - Got color: [%i, %i, %i]\n", color.r, color.g, color.b);

    if (effects.getCurrentEffect() == Effects::Effect::NullEffect) {
        effects.setCurrentCommand(Effects::Command::Color);
    }
    else {
        effects.setStartHue(color.h);
    }
}

bool Light::isMaster()
{
    return master;
}

bool Light::isMaster(bool m)
{
    master = m;
    return master;
}