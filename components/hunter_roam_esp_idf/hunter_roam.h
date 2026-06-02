#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace hunter_roam {

class HunterRoam : public Component {
 public:
  void set_pin(GPIOPin *pin);

  std::string errorHint(uint8_t error);

  uint8_t startZone(uint8_t zone, uint8_t time);
  uint8_t stopZone(uint8_t zone);
  uint8_t startProgram(uint8_t num);

 protected:
  GPIOPin *pin_;

  void sendLow();
  void sendHigh();
  void writeBus(std::vector<uint8_t> buffer, bool extrabit);
  void hunterBitfield(std::vector<uint8_t> &bits, uint8_t pos, uint8_t val, uint8_t len);
};

}  // namespace hunter_roam
}  // namespace esphome
