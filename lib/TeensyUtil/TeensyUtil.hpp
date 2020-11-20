#ifndef TEENSYUTIL_HPP
#define TEENSYUTIL_HPP

namespace Util {
#ifdef TEENSY
void restart() {
  // saveStatus();
  // send reboot command -----
  Serial.println("[util] Restarting teensy.");
  delay(1000);
  SCB_AIRCR = 0x05FA0004;
}
#endif
}  // namespace Util

#endif  // TEENSYUTIL_HPP