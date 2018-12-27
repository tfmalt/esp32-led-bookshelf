#ifndef LightController_h
#define LightController_h
#include <FastLED.h>

class Light {
    public:
        enum Segment {
            Top,
            Bottom,
            Left,
            Right
        };

        Light();

        Light& addLeds(CRGB* l);
        Light& addSegment(Segment segment, uint16_t start, uint16_t stop);
    private:
        CRGB*    leds;
        uint16_t topStart;
        uint16_t topStop;
        uint16_t bottomStart;
        uint16_t bottomStop;
};

#endif // LightController_h