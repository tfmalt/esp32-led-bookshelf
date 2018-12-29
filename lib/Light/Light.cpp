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

    state.initialize(filename.c_str());
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