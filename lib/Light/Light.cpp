#include "Light.h"
#include <FastLED.h>

Light::Light()
{
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