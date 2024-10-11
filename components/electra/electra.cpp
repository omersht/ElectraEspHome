#include "electra.h"
#include "esphome/core/log.h"

namespace esphome {
namespace electra {

static const char *const TAG = "electra.climate";

#define ELECTRA_TIME_UNIT 1000
#define ELECTRA_NUM_BITS 34

typedef enum IRElectraMode {
    IRElectraModeCool = 0b001,
    IRElectraModeHeat = 0b010,
    IRElectraModeAuto = 0b011,
    IRElectraModeDry  = 0b100,
    IRElectraModeFan  = 0b101,
    IRElectraModeOff  = 0b111
} IRElectraMode;


typedef enum IRElectraFan {
    IRElectraFanLow    = 0b00,
    IRElectraFanMedium = 0b01,
    IRElectraFanHigh   = 0b10,
    IRElectraFanAuto   = 0b11
} IRElectraFan;

typedef union ElectraCode {
 // That configuration has a total of 34 bits
 //    33: Power bit, if this bit is ON, the A/C will toggle it's power.
 // 32-30: Mode - Cool, heat etc.
 // 29-28: Fan - Low, medium etc.
 // 27-26: Zeros
 //    25: Swing On/Off
 //    24: iFeel On/Off
 //    23: Zero
 // 22-19: Temperature, where 15 is 0000, 30 is 1111
 //    18: Sleep mode On/Off
 // 17- 2: Zeros
 //     1: One
 //     0: Zero
    uint64_t num;
    struct {
        uint64_t zeros1 : 1;
        uint64_t ones1 : 1;
        uint64_t zeros2 : 16;
        uint64_t sleep : 1;
        uint64_t temperature : 4;
        uint64_t zeros3 : 1;
        uint64_t ifeel : 1;
        uint64_t swing : 1;
        uint64_t zeros4 : 2;
        uint64_t fan : 2;
        uint64_t mode : 3;
        uint64_t power : 1;
    };
} ElectraCode;


void ElectraClimate::setup() {
  climate_ir::ClimateIR::setup();

  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->active_mode_ = this->mode;
  }
}

void ElectraClimate::control(const climate::ClimateCall &call) {
  climate_ir::ClimateIR::control(call);
  this->active_mode_ = this->mode;
}

void ElectraClimate::setOffSupport(bool supports){
  if (supports){
    this->supportsOff = true;
  }
  else {
    this->supportsOff = false;
  }
}

void ElectraClimate::transmit_state() {
  if (this->active_mode_ != climate::CLIMATE_MODE_OFF || this->mode != climate::CLIMATE_MODE_OFF){
    ElectraCode code = { 0 };
    code.ones1 = 1;
 // original before the switch    code.fan = IRElectraFan::IRElectraFanAuto;

 // Set swing bit based on the swing mode
    if (this->swing_mode) {
      code.swing = 1;  // Swing ON
    } else {
      code.swing = 0;  // Swing OFF
    }
	
 /// below is for adding the fan mode
    switch (this->fan_mode.value()) {
      case climate::CLIMATE_FAN_LOW:
        code.fan = IRElectraFan::IRElectraFanLow;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        code.fan = IRElectraFan::IRElectraFanMedium;
        break;
      case climate::CLIMATE_FAN_HIGH:
        code.fan = IRElectraFan::IRElectraFanHigh;
        break;
      case climate::CLIMATE_FAN_AUTO:  //Added this for Auto Fan Mode
        code.fan = IRElectraFan::IRElectraFanAuto;
        break;
      default:
        code.fan = IRElectraFan::IRElectraFanAuto;// original IRElectraFanAuto
        break;
    }
 /// above is for adding the fan mode

    switch (this->mode) {
      case climate::CLIMATE_MODE_COOL:
        code.mode = IRElectraMode::IRElectraModeCool;
        code.power = this->active_mode_ == climate::CLIMATE_MODE_OFF ? 1 : 0;
        break;
      case climate::CLIMATE_MODE_HEAT:
        code.mode = IRElectraMode::IRElectraModeHeat;
        code.power = this->active_mode_ == climate::CLIMATE_MODE_OFF ? 1 : 0;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        code.mode = IRElectraMode::IRElectraModeFan;
        code.power = this->active_mode_ == climate::CLIMATE_MODE_OFF ? 1 : 0;
        break;
      case climate::CLIMATE_MODE_AUTO:
        code.mode = IRElectraMode::IRElectraModeAuto;
        code.power = this->active_mode_ == climate::CLIMATE_MODE_OFF ? 1 : 0;
        break;
      case climate::CLIMATE_MODE_DRY:
        code.mode = IRElectraMode::IRElectraModeDry;
        code.power = this->active_mode_ == climate::CLIMATE_MODE_OFF ? 1 : 0;
        break;
      case climate::CLIMATE_MODE_OFF:
      default:
        if (supportsOff){
          code.mode = IRElectraMode::IRElectraModeOff;
          code.power = 1;
        }else {
          code.mode = IRElectraMode::IRElectraModeCool;
          code.power = 1;
        }
        break;
    }
    auto temp = (uint8_t) roundf(clamp(this->target_temperature, this->minimum_temperature_, this->maximum_temperature_));
    code.temperature = temp - 15;

    ESP_LOGD(TAG, "Sending electra code: %lld", code.num);

    auto transmit = this->transmitter_->transmit();
    auto data = transmit.get_data();

    data->set_carrier_frequency(38000);
    uint16_t repeat = 3;

    for (uint16_t r = 0; r < repeat; r++) {
      // Header
      data->mark(3 * ELECTRA_TIME_UNIT);
      uint16_t next_value = 3 * ELECTRA_TIME_UNIT;
      bool is_next_space = true;

      // Data
      for (int j = ELECTRA_NUM_BITS - 1; j>=0; j--)
      {
        uint8_t bit = (code.num >> j) & 1;

        // if current index is SPACE
        if (is_next_space) {
          // one is one unit low, then one unit up
          // since we're pointing at SPACE, we should increase it by a unit
          // then add another MARK unit
          if (bit == 1) {
            data->space(next_value + ELECTRA_TIME_UNIT);
            next_value = ELECTRA_TIME_UNIT;
            is_next_space = false;

          } else {
            // we need a MARK unit, then SPACE unit
            data->space(next_value);
            data->mark(ELECTRA_TIME_UNIT);
            next_value = ELECTRA_TIME_UNIT;
            is_next_space = true;
          }

        } else {
          // current index is MARK
          
          // one is one unit low, then one unit up
          if (bit == 1) {
            data->mark(next_value);
            data->space(ELECTRA_TIME_UNIT);
            next_value = ELECTRA_TIME_UNIT;
            is_next_space = false;

          } else {
            data->mark(next_value + ELECTRA_TIME_UNIT);
            next_value = ELECTRA_TIME_UNIT;
            is_next_space = true;
          }
        }
      }

      // Last value must be SPACE
      data->space(next_value);
    }

    // Footer
    data->mark(4 * ELECTRA_TIME_UNIT);

    transmit.perform();
  }
} // end transmit state


bool ElectraClimate::on_receive(remote_base::RemoteReceiveData data){
  return false;
}



} // name space electra
}// namespace esphome