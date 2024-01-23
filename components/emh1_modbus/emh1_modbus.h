#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace emh1_modbus {

struct eMH1MessageT {
  uint8_t DeviceId;
	uint8_t FunctionCode;
	uint16_t Destination;
	uint16_t DataLength;
	uint8_t LRC;
	uint8_t WriteBytes;
	uint8_t WriteData[100];
};

class eMH1ModbusDevice;

class eMH1Modbus : public uart::UARTDevice, public Component {
 public:
  eMH1Modbus() = default;

  void setup() override;
  void loop() override;

  void dump_config() override;

  void register_device(eMH1ModbusDevice *device) { this->devices_.push_back(device); }
  void set_flow_control_pin(GPIOPin *flow_control_pin) { this->flow_control_pin_ = flow_control_pin; }

  float get_setup_priority() const override;

  void send(eMH1MessageT *tx_message);
  void query_status_report(uint8_t address);
  void query_device_info(uint8_t address);
  void query_config_settings(uint8_t address);
  void discover_devices();

 protected:
  bool parse_emh1_modbus_byte_(uint8_t byte);
  GPIOPin *flow_control_pin_{GPIO5};

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_emh1_modbus_byte_{0};
  std::vector<eMH1ModbusDevice *> devices_;
};

class eMH1ModbusDevice {
 public:
  void set_parent(eMH1Modbus *parent) { parent_ = parent; }
  void set_address(uint8_t address) { address_ = address; }
//  void set_serial_number(uint8_t *serial_number) { serial_number_ = serial_number; }
  virtual void on_emh1_modbus_data(const uint8_t &function, const std::vector<uint8_t> &data) = 0;

  void query_status_report(uint8_t address) { this->parent_->query_status_report(address); }
  void query_device_info(uint8_t address) { this->parent_->query_device_info(address); }
  void query_config_settings(uint8_t address) { this->parent_->query_config_settings(address); }
  void discover_devices() { this->parent_->discover_devices(); }

 protected:
  friend eMH1Modbus;

  eMH1Modbus *parent_;
  uint8_t address_;
//  uint8_t *serial_number_;
};

}  // namespace emh1_modbus
}  // namespace esphome
