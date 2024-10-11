#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace electra {

    // Temperature
const float RC3_TEMP_MIN = 16.0;
const float RC3_TEMP_MAX = 30.0;

class ElectraClimate : public climate_ir::ClimateIR {
 public:
  ElectraClimate()
      : climate_ir::ClimateIR(RC3_TEMP_MIN, RC3_TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {
                              }

  void setup() override;
  void setOffSupport(bool supports);
  void sync_state();


 protected:
  /// declartion of supports off command
  bool supportsOff;
  /// declartion of active mode value
  climate::ClimateMode active_mode_;
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  /// override control
  void control(const climate::ClimateCall &call) override;
};




}  // namespace electra
}  //namespace esphome