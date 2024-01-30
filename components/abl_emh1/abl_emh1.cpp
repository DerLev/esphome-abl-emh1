#include "abl_emh1.h"
#include "esphome/core/log.h"

namespace esphome {
namespace abl_emh1 {

static const char *const TAG = "abl_emh1";

static const uint8_t FUNCTION_STATUS_REPORT = 0x002E;
static const uint8_t FUNCTION_DEVICE_INFO = 0x002C;
static const uint8_t FUNCTION_CONFIG_SETTINGS = 0x0001;
static const uint8_t FUNCTION_DISCOVER_DEVICES = 0x0003; // 0x0050;

static const uint8_t MODES_SIZE = 7;
static const std::string MODES[MODES_SIZE] = {
    "Wait",             // 0
    "Check",            // 1
    "Normal",           // 2
    "Fault",            // 3
    "Permanent Fault",  // 4
    "Update",           // 5
    "Self Test",        // 6
};

static const uint8_t STATE_SIZE = 13;
static const char *const STATE[STATE_SIZE] = {
	"Waiting for EV",													// A1
	"EV is asking for charging", 							// B1
	"EV has the permission to charge",				// B2
	"EV is charging",													// C2
	"C2, reduced current (error F16, F17)",		// C3
	"C2, reduced current (imbalance F15)",		// C4
	"Outlet disabled",												// E0
	"Production test",												// E1
	"EVCC setup mode",												// E2
	"Bus idle",																// E3
	"Unintended closed contact (Welding)",		// F1
	"Internal error",													// F2
  "Unknown State code"											// default
};
static const char STATECODE[STATE_SIZE] = {
  0xA1, 0xB1, 0xB2, 0xC2, 0xC3, 0xc4, 
	0xE0, 0xE1, 0xE2, 0xE3, 0xF1, 0xF2, 0x00
};

void ABLeMH1::on_emh1_modbus_data(uint16_t function, uint16_t datalength, const uint8_t* data) {
  switch (function) {
    case FUNCTION_DEVICE_INFO:
      this->decode_device_info_(data, datalength);
      break;
    case FUNCTION_STATUS_REPORT:
      this->decode_status_report_(data, datalength);
      break;
    case FUNCTION_CONFIG_SETTINGS:
      this->decode_config_settings_(data, datalength);
      break;
    case FUNCTION_DISCOVER_DEVICES:
      this->decode_serial_number_(data, datalength);
      break;
    default:
      // ESP_LOGW(TAG, "Unhandled ABL frame: %s", format_hex_pretty(&data.front(), data.size()).c_str());
      ESP_LOGW(TAG, "Unhandled ABL frame");
  }
}

void ABLeMH1::decode_device_info_(const uint8_t* data, uint16_t datalength) {
  ESP_LOGI(TAG, "Device info frame received");
  //ESP_LOGI(TAG, "  Device type: %d", data[0]);
  //ESP_LOGI(TAG, "  Rated power: %s", std::string(data.begin() + 1, data.begin() + 1 + 6).c_str());
  //ESP_LOGI(TAG, "  Firmware version: %s", std::string(data.begin() + 7, data.begin() + 7 + 5).c_str());
  //ESP_LOGI(TAG, "  Module name: %s", std::string(data.begin() + 12, data.begin() + 12 + 14).c_str());
  //ESP_LOGI(TAG, "  Manufacturer: %s", std::string(data.begin() + 26, data.begin() + 26 + 14).c_str());
  //ESP_LOGI(TAG, "  Serial number: %s", std::string(data.begin() + 40, data.begin() + 40 + 14).c_str());
  //ESP_LOGI(TAG, "  Rated bus voltage: %s", std::string(data.begin() + 54, data.begin() + 54 + 4).c_str());
  this->no_response_count_ = 0;
}

void ABLeMH1::decode_config_settings_(const uint8_t* data, uint16_t datalength) {
  //if (data.size() != 68) {
  //  ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
  //  return;
  //}

//  auto emh1_get_16bit = [&](size_t i) -> uint16_t {
//    return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
//  };

  ESP_LOGI(TAG, "Config settings frame received");
  //ESP_LOGI(TAG, "  wVpvStart [9.10]: %f V", emh1_get_16bit(0) * 0.1f);
  //ESP_LOGI(TAG, "  wTimeStart [11.12]: %d S", emh1_get_16bit(2));
  //ESP_LOGI(TAG, "  wVacMinProtect [13.14]: %f V", emh1_get_16bit(4) * 0.1f);

  this->no_response_count_ = 0;
}

void ABLeMH1::decode_status_report_(const uint8_t* data, uint16_t datalength) {
  ESP_LOGI(TAG, "Status frame received");
	if (data[0] != 0x2E) {
	  ESP_LOGD(TAG, "Expected data[0] to be 0x2E");
		return;
	}
	uint8_t x;
	for (x=0; x < STATE_SIZE; x++) {
	  if (data[1] == STATECODE[x]) break;
	}
  this->publish_state_(this->mode_sensor_, STATECODE[x]);
  this->publish_state_(this->mode_name_text_sensor_, STATE[x]);
	this->publish_state_(this->errors_text_sensor_, "");
  this->publish_state_(this->l1_current_sensor_, 
  	((data[4] << 4) + data[5]) / 10.0);
  this->publish_state_(this->l2_current_sensor_,
    ((data[6] << 4) + data[7]) / 10.0);
  this->publish_state_(this->l3_current_sensor_, 
  	((data[8] << 4) + data[9]) / 10.0);
  this->publish_state_(this->max_current_sensor_, 
  	(((data[2] & 0x0F) << 4) + data[3]) * 10.0);
  this->publish_state_(this->en1_status_sensor_, (data[2] & 0x10) >> 4);
  this->publish_state_(this->en2_status_sensor_, (data[2] & 0x20) >> 5);
  this->publish_state_(this->duty_cycle_reduced_, (data[2] & 0x40) >> 6);
  this->publish_state_(this->ucp_status_sensor_, (data[2] & 0x80) >> 7);
  // this->publish_state_(this->serial_number_sensor_, NAN);
  // this->publish_state_(this->outlet_state_sensor_, NAN);
  // this->publish_state_(this->mode_name_text_sensor_, "Online");
	// this->publish_state_(this->errors_text_sensor_, "Connected");

  //if (data.size() != 52 && data.size() != 50 && data.size() != 56) {
    // Solax X1 mini status report (data_len 0x34: 52 bytes):
    // AA.55.00.0A.01.00.11.82.34.00.1A.00.02.00.00.00.00.00.00.00.00.00.00.09.21.13.87.00.00.FF.FF.
    // 00.00.00.12.00.00.00.15.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.04.D6

    // Solax X1 mini g2 status report (data_len 0x32: 50 bytes):
    // AA.55.00.0A.01.00.11.82.32.00.21.00.02.07.EC.00.00.00.1D.00.00.00.18.09.55.13.80.02.2B.FF.FF.
    // 00.00.5D.AF.00.00.10.50.00.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.07.A4

    // Solax X1 mini g3 status report (data_len 0x38: 56 bytes):
    // AA.55.00.0A.01.00.11.82.38.00.1A.00.03.04.0C.00.00.00.19.00.00.00.0B.08.FC.13.8A.00.F8.FF.FF.
    // 00.00.00.2B.00.00.00.0D.00.02.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.8A.00.DE.08.5F
    //ESP_LOGW(TAG, "Invalid response size: %zu", data.size());
    //ESP_LOGW(TAG, "Your device is probably not supported. Please create an issue here: "
    //              "https://github.com/syssi/esphome-solax-x1-mini/issues");
    //ESP_LOGW(TAG, "Please provide the following status response data: %s",
    //         format_hex_pretty(&data.front(), data.size()).c_str());
    //return;
  //}

//  auto emh1_get_16bit = [&](size_t i) -> uint16_t {
//    return (uint16_t(data[i + 0]) << 8) | (uint16_t(data[i + 1]) << 0);
//  };
//  auto emh1_get_32bit = [&](size_t i) -> uint32_t {
//    return uint32_t((data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3]);
//  };
//  auto emh1_get_error_bitmask = [&](size_t i) -> uint32_t {
//    return uint32_t((data[i + 3] << 24) | (data[i + 2] << 16) | (data[i + 1] << 8) | data[i]);
//  };


//  this->publish_state_(this->temperature_sensor_, (int16_t) emh1_get_16bit(0));
//  this->publish_state_(this->energy_today_sensor_, emh1_get_16bit(2) * 0.1f);
//  this->publish_state_(this->dc1_voltage_sensor_, emh1_get_16bit(4) * 0.1f);
//  this->publish_state_(this->dc2_voltage_sensor_, emh1_get_16bit(6) * 0.1f);
//  this->publish_state_(this->dc1_current_sensor_, emh1_get_16bit(8) * 0.1f);
//  this->publish_state_(this->dc2_current_sensor_, emh1_get_16bit(10) * 0.1f);
//  this->publish_state_(this->ac_current_sensor_, emh1_get_16bit(12) * 0.1f);
//  this->publish_state_(this->ac_voltage_sensor_, emh1_get_16bit(14) * 0.1f);
//  this->publish_state_(this->ac_frequency_sensor_, emh1_get_16bit(16) * 0.01f);
//  this->publish_state_(this->ac_power_sensor_, emh1_get_16bit(18));

  // register 20 is not used

//  uint32_t raw_energy_total = emh1_get_32bit(22);
  // The inverter publishes a zero once per day on boot-up. This confuses the energy dashboard.
//  if (raw_energy_total > 0) {
    // this->publish_state_(this->energy_total_sensor_, raw_energy_total * 0.1f);
//  }

//  uint32_t raw_runtime_total = emh1_get_32bit(26);
//  if (raw_runtime_total > 0) {
    // this->publish_state_(this->runtime_total_sensor_, (float) raw_runtime_total);
//  }

 // uint8_t mode = (uint8_t) emh1_get_16bit(30);
  // this->publish_state_(this->mode_sensor_, mode);
  // this->publish_state_(this->mode_name_text_sensor_, (mode < MODES_SIZE) ? MODES[mode] : "Unknown");

  ///this->publish_state_(this->grid_voltage_fault_sensor_, emh1_get_16bit(32) * 0.1f);
  //this->publish_state_(this->grid_frequency_fault_sensor_, emh1_get_16bit(34) * 0.01f);
  //this->publish_state_(this->dc_injection_fault_sensor_, emh1_get_16bit(36) * 0.001f);
  //this->publish_state_(this->temperature_fault_sensor_, (float) emh1_get_16bit(38));
  //this->publish_state_(this->pv1_voltage_fault_sensor_, emh1_get_16bit(40) * 0.1f);
  //this->publish_state_(this->pv2_voltage_fault_sensor_, emh1_get_16bit(42) * 0.1f);
  //this->publish_state_(this->gfc_fault_sensor_, emh1_get_16bit(44) * 0.001f);

//  uint32_t error_bits = emh1_get_error_bitmask(46);
  //this->publish_state_(this->error_bits_sensor_, error_bits);
  //this->publish_state_(this->errors_text_sensor_, this->error_bits_to_string_(error_bits));

//  if (data.size() > 50) {
//    ESP_LOGD(TAG, "  CT Pgrid: %d W", emh1_get_16bit(50));
//  }

  this->no_response_count_ = 0;
}

void ABLeMH1::decode_serial_number_(const uint8_t* data, uint16_t datalength) {
  
  this->no_response_count_ = 0;
}


void ABLeMH1::publish_device_offline_() {
  this->publish_state_(this->mode_sensor_, -1);
  this->publish_state_(this->l1_current_sensor_, NAN);
  this->publish_state_(this->l2_current_sensor_, NAN);
  this->publish_state_(this->l3_current_sensor_, NAN);
  this->publish_state_(this->max_current_sensor_, NAN);
  this->publish_state_(this->en1_status_sensor_, NAN);
  this->publish_state_(this->en2_status_sensor_, NAN);
  this->publish_state_(this->duty_cycle_reduced_, NAN);
  this->publish_state_(this->ucp_status_sensor_, NAN);
  this->publish_state_(this->serial_number_sensor_, NAN);
  this->publish_state_(this->outlet_state_sensor_, NAN);
  this->publish_state_(this->mode_name_text_sensor_, "Offline");
	this->publish_state_(this->errors_text_sensor_, "Offline");
}

void ABLeMH1::update() {
  if (this->no_response_count_ >= REDISCOVERY_THRESHOLD) {
    this->publish_device_offline_();
    ESP_LOGD(TAG, "The device is or was offline. Broadcasting discovery for address configuration...");
    this->discover_devices();
    //    this->query_device_info(this->address_);
    // Try to query live data on next update again. The device doesn't
    // respond to the discovery broadcast if it's already configured.
    this->no_response_count_ = 0;
  } else {
    this->no_response_count_++;
    this->query_status_report(this->address_);
  }
}

void ABLeMH1::publish_state_(sensor::Sensor *sensor, float value) {
  if (sensor == nullptr)
    return;

  sensor->publish_state(value);
}

void ABLeMH1::publish_state_(text_sensor::TextSensor *text_sensor, const std::string &state) {
  if (text_sensor == nullptr)
    return;

  text_sensor->publish_state(state);
}

void ABLeMH1::dump_config() {
  ESP_LOGCONFIG(TAG, "ABLeMH1:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", this->address_);
//  LOG_SENSOR("", "Temperature", this->temperature_sensor_);
//  LOG_SENSOR("", "Energy today", this->energy_today_sensor_);
//  LOG_SENSOR("", "DC1 voltage", this->dc1_voltage_sensor_);
//  LOG_SENSOR("", "DC2 voltage", this->dc2_voltage_sensor_);
//  LOG_SENSOR("", "DC1 current", this->dc1_current_sensor_);
//  LOG_SENSOR("", "DC2 current", this->dc2_current_sensor_);
//  LOG_SENSOR("", "AC current", this->ac_current_sensor_);
//  LOG_SENSOR("", "AC voltage", this->ac_voltage_sensor_);
//  LOG_SENSOR("", "AC frequency", this->ac_frequency_sensor_);
//  LOG_SENSOR("", "AC power", this->ac_power_sensor_);
//  LOG_SENSOR("", "Energy total", this->energy_total_sensor_);
//  LOG_SENSOR("", "Runtime total", this->runtime_total_sensor_);
//  LOG_SENSOR("", "Mode", this->mode_sensor_);
//  LOG_SENSOR("", "Error bits", this->error_bits_sensor_);
//  LOG_SENSOR("", "Grid voltage fault", this->grid_voltage_fault_sensor_);
//  LOG_SENSOR("", "Grid frequency fault", this->grid_frequency_fault_sensor_);
//  LOG_SENSOR("", "DC injection fault", this->dc_injection_fault_sensor_);
//  LOG_SENSOR("", "Temperature fault", this->temperature_fault_sensor_);
//  LOG_SENSOR("", "PV1 voltage fault", this->pv1_voltage_fault_sensor_);
//  LOG_SENSOR("", "PV2 voltage fault", this->pv2_voltage_fault_sensor_);
//  LOG_SENSOR("", "GFC fault", this->gfc_fault_sensor_);
//  LOG_TEXT_SENSOR("  ", "Mode name", this->mode_name_text_sensor_);
//  LOG_TEXT_SENSOR("  ", "Errors", this->errors_text_sensor_);
}

std::string ABLeMH1::error_bits_to_string_(const uint32_t mask) {
  std::string values = "";
  if (mask) {
    for (int i = 0; i < STATE_SIZE; i++) {
      if (mask & (1 << i)) {
        values.append(STATE[i]);
        values.append(";");
      }
    }
    if (!values.empty()) {
      values.pop_back();
    }
  }
  return values;
}

}  // namespace abl_emh1
}  // namespace esphome
