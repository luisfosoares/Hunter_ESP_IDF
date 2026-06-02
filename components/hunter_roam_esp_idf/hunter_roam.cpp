#include "hunter_roam.h"
#include "esphome/core/log.h"

#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome {
namespace hunter_roam {

static const char *TAG = "hunter_roam";

// Timing (tune if needed)
static const int SHORT_INTERVAL = 300;
static const int LONG_INTERVAL  = 900;
static const int START_INTERVAL = 2000;

void HunterRoam::set_pin(GPIOPin *pin) {
  this->pin_ = pin;
  this->pin_->setup();
}

std::string HunterRoam::errorHint(uint8_t error) {
  switch (error) {
    case 0: return "No error.";
    case 1: return "Invalid zone number.";
    case 2: return "Invalid watering time.";
    case 3: return "Invalid program number.";
    default: return "Unknown error.";
  }
}

void HunterRoam::sendLow() {
  this->pin_->digital_write(true);
  esp_rom_delay_us(SHORT_INTERVAL);
  this->pin_->digital_write(false);
  esp_rom_delay_us(LONG_INTERVAL);
}

void HunterRoam::sendHigh() {
  this->pin_->digital_write(true);
  esp_rom_delay_us(LONG_INTERVAL);
  this->pin_->digital_write(false);
  esp_rom_delay_us(SHORT_INTERVAL);
}

void HunterRoam::writeBus(std::vector<uint8_t> buffer, bool extrabit) {
  // Reset pulse
  this->pin_->digital_write(true);
  vTaskDelay(pdMS_TO_TICKS(325));
  this->pin_->digital_write(false);
  vTaskDelay(pdMS_TO_TICKS(65));

  // Start pulse
  this->pin_->digital_write(true);
  esp_rom_delay_us(START_INTERVAL);
  this->pin_->digital_write(false);
  esp_rom_delay_us(SHORT_INTERVAL);

  // Send data
  for (auto b : buffer) {
    for (uint8_t i = 0; i < 8; i++) {
      (b & 0x80) ? sendHigh() : sendLow();
      b <<= 1;
    }
  }

  if (extrabit) {
    sendHigh();
  }

  sendLow();  // Stop pulse
}

void HunterRoam::hunterBitfield(std::vector<uint8_t> &bits, uint8_t pos, uint8_t val, uint8_t len) {
  while (len > 0) {
    if (val & 0x01)
      bits[pos / 8] |= (0x80 >> (pos % 8));
    else
      bits[pos / 8] &= ~(0x80 >> (pos % 8));

    val >>= 1;
    pos++;
    len--;
  }
}

uint8_t HunterRoam::startZone(uint8_t zone, uint8_t time) {
  std::vector<uint8_t> buffer = {
    0xff,0x00,0x00,0x00,0x10,0x00,0x00,
    0x04,0x00,0x00,0x01,0x00,0x01,0xb8,0x3f
  };

  if (zone < 1 || zone > 48) return 1;
  if (time > 240) return 2;

  if (zone > 12)
    hunterBitfield(buffer, 9, 0x1, 2);
  else
    hunterBitfield(buffer, 9, 0x2, 2);

  hunterBitfield(buffer, 23, zone + 0x17, 7);
  hunterBitfield(buffer, 36, zone + 0x17, 7);
  hunterBitfield(buffer, 49, zone + 0x23, 7);
  hunterBitfield(buffer, 62, zone + 0x23, 7);
  hunterBitfield(buffer, 75, zone + 0x2f, 7);
  hunterBitfield(buffer, 88, zone + 0x2f, 7);

  hunterBitfield(buffer, 31, time, 4);
  hunterBitfield(buffer, 44, time >> 4, 4);
  hunterBitfield(buffer, 57, time, 4);
  hunterBitfield(buffer, 70, time >> 4, 4);
  hunterBitfield(buffer, 83, time, 4);
  hunterBitfield(buffer, 96, time >> 4, 4);

  hunterBitfield(buffer, 109, zone - 1, 4);

  writeBus(buffer, true);
  return 0;
}

uint8_t HunterRoam::stopZone(uint8_t zone) {
  return startZone(zone, 0);
}

uint8_t HunterRoam::startProgram(uint8_t num) {
  std::vector<uint8_t> buffer = {
    0xff, 0x40, 0x03, 0x96, 0x09, 0xbd, 0x7f
  };

  if (num < 1 || num > 4) return 3;

  hunterBitfield(buffer, 31, num - 1, 2);
  writeBus(buffer, false);
  return 0;
}

}  // namespace hunter_roam
}  // namespace esphome
