#include "ElectraClimate.h"
#include "esphome.h"

ElectraClimate::ElectraClimate() 
    : active_mode_(climate::CLIMATE_MODE_OFF), // Default mode
      target_temperature(24), // Default target temperature
      swing_mode(climate::CLIMATE_SWING_OFF), // Default swing mode
    fan_mode(climate::CLIMATE_FAN_AUTO),{ // Default fan mode

}

void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
    
    this->transmitter_ = transmitter;
  }

void ElectraClimate::setup() {
    if (this->sensor_) {
      this->sensor_->add_on_state_callback([this](float state) {
        this->current_temperature = state;

        // current temperature changed, publish state
        this->publish_state();
      });
      this->current_temperature = this->sensor_->state;
    } else
      this->current_temperature = NAN;

    // restore set points
    auto restore = this->restore_state_();
    if (restore.has_value()) {
      restore->apply(this);
    } else {
      // restore from defaults to be off (before was CLIMATE_MODE_AUTO
      this->mode = climate::CLIMATE_MODE_OFF;

      this->swing_mode = climate::CLIMATE_SWING_OFF;

      // initialize target temperature to some value so that it's not NAN
      this->target_temperature = 24;
    }

    this->active_mode_ = this->mode;
} // end setup


void ElectraClimate::control(const climate::ClimateCall &call) {
//Handle HVAC mode
    if (call.get_mode().has_value())
      this->mode = *call.get_mode();
//Hendle temp
    if (call.get_target_temperature().has_value())
      this->target_temperature = *call.get_target_temperature();
//Handle Fan modes
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();
// Handle Swing Mode
  if (call.get_swing_mode().has_value()) {
    if (*call.get_swing_mode() == climate::CLIMATE_SWING_OFF)
      this->swing_mode = climate::CLIMATE_SWING_OFF;
    else
      this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  }


    this->transmit_state_();
    this->publish_state();

    this->active_mode_ = this->mode;
} // end control



void ElectraClimate::transmit_state_() {
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
        code.mode = IRElectraMode::IRElectraModeOff;
        code.power = 1;
        break;
    }
    target = (uint8_t) this->target_temperature;
    auto temp = (uint8_t) roundf(clamp(target, ELECTRA_TEMP_MIN, ELECTRA_TEMP_MAX));
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

climate::ClimateTraits ElectraClimate::traits(){
    auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(this->sensor_ != nullptr);
  traits.set_supported_modes({ climate::CLIMATE_MODE_AUTO, climate::CLIMATE_MODE_COOL, climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_FAN_ONLY, climate::CLIMATE_MODE_DRY, climate::CLIMATE_MODE_OFF });
  
// added the below line for having support for the fans modes as from climate_traits.h
  traits.set_supported_fan_modes({ climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH });
// added support for swing modes
  traits.set_supported_swing_modes({ climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL });

  traits.set_supports_two_point_target_temperature(false);
  //traits.set_supports_away(false);
  traits.set_visual_min_temperature(ELECTRA_TEMP_MIN);
  traits.set_visual_max_temperature(ELECTRA_TEMP_MAX);
  traits.set_visual_temperature_step(1);
  return traits;
}