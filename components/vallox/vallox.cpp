#include "vallox.h"
#include "esphome/core/log.h"

namespace esphome {
	namespace vallox {
		static const char *TAG = "vallox";

/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilationHeatBypassNum::control(float value) {
			if (value >= 0 && value <= 20) {
				unsigned char hex = this->parent_->convCel2Ntc(value);
				this->parent_->setVariable(VX_VARIABLE_T_HEAT_BYPASS,hex);
				this->publish_state(value);
				this->parent_->requestVariable(VX_VARIABLE_T_HEAT_BYPASS);
			}
		}

/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilationServiceResetBtn::press_action() {
			// use service_period to set...
			this->parent_->setVariable(VX_VARIABLE_SERVICE_REMAINING, (unsigned char) this->parent_->service_period);
			this->parent_->requestVariable(VX_VARIABLE_SERVICE_REMAINING);
		}

/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilationSwitchTypeSelectSel::control(const std::string &value) {
			if (value == SWITCH_TYPE_ACTION_BOOST) {
				this->parent_->setFlags(VX_VARIABLE_PROGRAM,VX_PROGRAM_SWITCH_TYPE,1); // set bit 5 to 1
			}
			if (value == SWITCH_TYPE_ACTION_FIREPLACE) {
				this->parent_->setFlags(VX_VARIABLE_PROGRAM,VX_PROGRAM_SWITCH_TYPE,0); // set bit 5 to 0

			}
			this->parent_->requestVariable(VX_VARIABLE_PROGRAM);
			this->parent_->requestVariable(VX_VARIABLE_IO_08);
		}

/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilationFanSpeedMaxNum::control(float value) {
			if (value >= 1 && value <= 8) {
				unsigned char hex = ~(((unsigned char)0xFF) << (int)value);  // example for speed 2 : 11111111 => 11111100 => 00000011 => 0x03
				this->parent_->setVariable(VX_VARIABLE_FAN_SPEED_MAX,hex);
				this->publish_state(value);
			}
		}

/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilationFanSpeedMinNum::control(float value) {
			if (value >= 1 && value <= 8) {
				unsigned char hex = ~(((unsigned char)0xFF) << (int)value);
				this->parent_->setVariable(VX_VARIABLE_FAN_SPEED_MIN,hex);
				this->publish_state(value);
				this->parent_->requestVariable(VX_VARIABLE_FAN_SPEED_MIN); // update dependent sensors immediately (fan_speed_default_sensor_)
			}
		}

/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilationSwitchBtn::press_action() {
			this->parent_->setFlags(VX_VARIABLE_FLAGS_06,VX_06_FIREPLACE_FLAG_ACTIVATE,true);
			this->parent_->requestVariable(VX_VARIABLE_IO_08);
			this->parent_->requestVariable(VX_VARIABLE_FLAGS_06);
			this->parent_->requestVariable(VX_VARIABLE_SWITCH_REMAINING);
			// this action is triggered rarely so we don't care if we request the variable without checking if we need it... same goes for the others above..
		}

/////////////////////////////////////////////////////////////////////////////////////////


		// sets available controls in climate control in HA
		climate::ClimateTraits ValloxVentilation::traits() {
			auto traits = climate::ClimateTraits();
			traits.set_supports_action(true);
			traits.set_supports_current_temperature(true);
			traits.set_supports_two_point_target_temperature(false);
			traits.set_supports_current_humidity(false);
			traits.set_supports_target_humidity(false);
			traits.set_visual_min_temperature(CLIMATE_MIN_TEMPERATURE);
			traits.set_visual_max_temperature(CLIMATE_MAX_TEMPERATURE);
			traits.set_visual_temperature_step(CLIMATE_TEMPERATURE_STEP);
			traits.set_supported_custom_fan_modes(preset_custom_fan_modes);
			traits.set_supported_modes(
			{
				climate::ClimateMode::CLIMATE_MODE_OFF,
				climate::ClimateMode::CLIMATE_MODE_HEAT,
				climate::ClimateMode::CLIMATE_MODE_FAN_ONLY
			}
			);
			return traits;
		}


/////////////////////////////////////////////////////////////////////////////////////////

		// initial output when connecting via API
		void ValloxVentilation::dump_config() {

			// module description
			ESP_LOGCONFIG(TAG, "Vallox Ventilation:");

			// log climate traits
			climate::Climate::dump_traits_(TAG);

			// log sensor details
			if (this->fan_speed_sensor_            != nullptr) { LOG_SENSOR("  ", "Sensor fan speed",               this->fan_speed_sensor_);            }
			if (this->fan_speed_default_sensor_    != nullptr) { LOG_SENSOR("  ", "Sensor fan speed (default)",     this->fan_speed_default_sensor_);    }
			if (this->temperature_target_sensor_   != nullptr) { LOG_SENSOR("  ", "Sensor temperature target",      this->temperature_target_sensor_);   }
			if (this->temperature_outside_sensor_  != nullptr) { LOG_SENSOR("  ", "Sensor temperature (outside)",   this->temperature_outside_sensor_);  }
			if (this->temperature_inside_sensor_   != nullptr) { LOG_SENSOR("  ", "Sensor temperature (inside)",    this->temperature_inside_sensor_);   }
			if (this->temperature_outgoing_sensor_ != nullptr) { LOG_SENSOR("  ", "Sensor temperature (outgoing)",  this->temperature_outgoing_sensor_); }
			if (this->temperature_incoming_sensor_ != nullptr) { LOG_SENSOR("  ", "Sensor temperature (incoming)",  this->temperature_incoming_sensor_); }
			if (this->humidity_1_sensor_           != nullptr) { LOG_SENSOR("  ", "Sensor humidity sensor 1",       this->humidity_1_sensor_);           }
			if (this->humidity_2_sensor_           != nullptr) { LOG_SENSOR("  ", "Sensor humidity sensor 2",       this->humidity_2_sensor_);           }
			if (this->co2_sensor_                  != nullptr) { LOG_SENSOR("  ", "Sensor CO2 sensor",              this->co2_sensor_);                  }
			if (this->service_period_sensor_       != nullptr) { LOG_SENSOR("  ", "Sensor service period",          this->service_period_sensor_);       }
			if (this->service_remaining_sensor_    != nullptr) { LOG_SENSOR("  ", "Sensor service remaining",       this->service_remaining_sensor_);    }
			if (this->switch_remaining_sensor_     != nullptr) { LOG_SENSOR("  ", "Sensor switch time remaining",   this->switch_remaining_sensor_);     }

			// log text sensor details
			if (this->switch_type_text_sensor_     != nullptr) { LOG_TEXT_SENSOR("  ", "Sensor switch type", this->switch_type_text_sensor_);         }
			if (this->fault_condition_text_sensor_ != nullptr) { LOG_TEXT_SENSOR("  ", "Sensor fault condition", this->fault_condition_text_sensor_); }

			// log binary sensor details
			if (this->status_on_binary_sensor_        != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor status (active)",        this->status_on_binary_sensor_);        }
			if (this->status_motor_in_binary_sensor_  != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor status motor incoming",  this->status_motor_in_binary_sensor_);  }
			if (this->status_motor_out_binary_sensor_ != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor status motor outgoing",  this->status_motor_out_binary_sensor_); }
			if (this->service_needed_binary_sensor_   != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor service required",       this->service_needed_binary_sensor_);   }
			if (this->switch_active_binary_sensor_    != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor switch active",          this->switch_active_binary_sensor_);    }
			if (this->heating_binary_sensor_          != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor heating (active)",       this->heating_binary_sensor_);          }
			if (this->front_heating_binary_sensor_    != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor front heating (active)", this->front_heating_binary_sensor_);    }
			if (this->heating_mode_binary_sensor_     != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor heating mode (active)",  this->heating_mode_binary_sensor_);     }
			if (this->summer_mode_binary_sensor_      != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor summer mode",            this->summer_mode_binary_sensor_);      }
			if (this->problem_binary_sensor_          != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor problem",                this->problem_binary_sensor_);          }
			if (this->error_relay_binary_sensor_      != nullptr) { LOG_BINARY_SENSOR("  ", "Sensor error relay",            this->error_relay_binary_sensor_);      }

			// log number details
			if (this->heat_bypass_number_   != nullptr) { LOG_NUMBER("  ", "Number heat bypass",   this->heat_bypass_number_);   }
			if (this->fan_speed_max_number_ != nullptr) { LOG_NUMBER("  ", "Number fan speed max", this->fan_speed_max_number_); }
			if (this->fan_speed_min_number_ != nullptr) { LOG_NUMBER("  ", "Number fan speed min", this->fan_speed_min_number_); }

			// log button details
			if (this->service_reset_button_ != nullptr) { LOG_BUTTON("  ", "Button service reset", this->service_reset_button_); }
			if (this->switch_button_        != nullptr) { LOG_BUTTON("  ", "Button switch",        this->switch_button_);        }

			// log select details
			if (this->switch_type_select_select_ != nullptr) { LOG_SELECT("  ", "Select switch type", this->switch_type_select_select_); }
		}

/////////////////////////////////////////////////////////////////////////////////////////

		// control requests from HA for climate settings
		void ValloxVentilation::control(const climate::ClimateCall &call) {
			unsigned char hex = 0x00;
			unsigned char target_status;
			int cel = 10;
			int speed = 1;

			if (call.get_mode().has_value()) {
				switch (*call.get_mode()) {
					case climate::CLIMATE_MODE_HEAT:
						target_status = buffer[VX_VARIABLE_STATUS] | VX_STATUS_FLAG_POWER;          // Turn on
						target_status = target_status | VX_STATUS_FLAG_HEATING_MODE;   // Set heating mode
						if (target_status != buffer[VX_VARIABLE_STATUS]) {
							setVariable(VX_VARIABLE_STATUS, target_status);
						}
						this->mode   = climate::CLIMATE_MODE_HEAT;
						this->action = climate::CLIMATE_ACTION_HEATING;
						break;
					case climate::CLIMATE_MODE_FAN_ONLY:
						target_status = buffer[VX_VARIABLE_STATUS] | VX_STATUS_FLAG_POWER;          // Turn on
						target_status = target_status & ~VX_STATUS_FLAG_HEATING_MODE;   // disable heating mode
						if (target_status != buffer[VX_VARIABLE_STATUS]) {
							setVariable(VX_VARIABLE_STATUS, target_status);
						}
						this->mode   = climate::CLIMATE_MODE_FAN_ONLY;
						this->action = climate::CLIMATE_ACTION_FAN;
						break;
					case climate::CLIMATE_MODE_OFF:
						target_status = buffer[VX_VARIABLE_STATUS] & ~VX_STATUS_FLAG_POWER;          // Turn off
						if (target_status != buffer[VX_VARIABLE_STATUS]) {
							setVariable(VX_VARIABLE_STATUS, target_status);
							buffer[VX_VARIABLE_STATUS] = target_status;  // set buffer to "turned off" to avoid instances of sending same "turn off" command twice, might be voodoo but read somewhere that this should be avoided...
						}
						this->mode   = climate::CLIMATE_MODE_OFF;
						this->action = climate::CLIMATE_ACTION_OFF;
						break;
					default:
					// should not happen
						break;
				}
			}

			// Set target temperature on device
			if (call.get_target_temperature().has_value()) {
				cel = (int) *call.get_target_temperature();
				if (cel >= 10 && cel <= 27) {
					hex = convCel2Ntc(cel);
					setVariable(VX_VARIABLE_HEATING_TARGET, hex);
					this->target_temperature = cel;
					if (this->temperature_target_sensor_   != nullptr) { requestVariable(VX_VARIABLE_HEATING_TARGET); } // immediately update other depending sensors
				}
			}
			// Set fan speed
			if (call.get_custom_fan_mode().has_value()) {
				speed = std::stoi(*call.get_custom_fan_mode());
				if (speed <= VX_MAX_FAN_SPEED) {
					hex = convFanSpeed2Hex(speed);
					setVariable(VX_VARIABLE_FAN_SPEED, hex);
					this->custom_fan_mode = (optional<std::string>) to_string(speed);
					if (this->fan_speed_sensor_ != nullptr) { requestVariable(VX_VARIABLE_FAN_SPEED); } // immediately update other depending sensors
				}
			}
			this->publish_state();
		}


/////////////////////////////////////////////////////////////////////////////////////////

		void ValloxVentilation::setup() {
			retryVariables();
		}


		void ValloxVentilation::retryVariables() {

			bool retrySlow = 0;
			bool retryFlag = 0;

			iofastqueryts = ionow;
			if ((ionow - ioslowqueryts) > SLOW_QUERY_INTERVAL) { retrySlow = 1; ioslowqueryts = ionow;}

			// 0x08 // fast query
			retryFlag = 0;
			if (this->summer_mode_binary_sensor_      != nullptr) { retryFlag = 1; }
			if (this->status_motor_in_binary_sensor_  != nullptr) { retryFlag = 1; }
			if (this->status_motor_out_binary_sensor_ != nullptr) { retryFlag = 1; }
			if (this->front_heating_binary_sensor_    != nullptr) { retryFlag = 1; }
			if (this->error_relay_binary_sensor_      != nullptr) { retryFlag = 1; }
			if (retryFlag)     { requestVariable(VX_VARIABLE_IO_08);  }
			// 0x29 // fast query // required by climate sensor
			requestVariable(VX_VARIABLE_FAN_SPEED);
			// 0x2B // 0x2C // no query needed, broadcast by mainboard
			// if (this->co2_sensor_ != nullptr) { if (!this->co2_sensor_->has_state())                 { requestVariable(VX_VARIABLE_CO2_LO); requestVariable(VX_VARIABLE_CO2_HI); }}
			// 0x2F // fast
			if (this->humidity_1_sensor_ != nullptr) { requestVariable(VX_VARIABLE_RH1); }
			// 0x30 // fast
			if (this->humidity_2_sensor_ != nullptr) { requestVariable(VX_VARIABLE_RH2); }
			// 0x32
			if (this->temperature_outside_sensor_  != nullptr) { if ((!this->temperature_outside_sensor_->has_state())  || retrySlow ) { requestVariable(VX_VARIABLE_T_OUTSIDE ); }}
			// 0x33
			if (this->temperature_outgoing_sensor_ != nullptr) { if ((!this->temperature_outgoing_sensor_->has_state()) || retrySlow ) { requestVariable(VX_VARIABLE_T_OUTGOING); }}
			// 0x34 // slow query // required by climate sensor
			if (!buffer.contains(VX_VARIABLE_T_INSIDE) || retrySlow ) { requestVariable(VX_VARIABLE_T_INSIDE); }
			// 0x35
			if (this->temperature_incoming_sensor_ != nullptr) { if ((!this->temperature_incoming_sensor_->has_state()) || retrySlow ) { requestVariable(VX_VARIABLE_T_INCOMING); }}
			// 0x36
			// do not request variable, mainboard will not answer (without fault? Does it get pushed?)
			// if (this->fault_condition_text_sensor_ != nullptr) { if (!this->fault_condition_text_sensor_->has_state()) { requestVariable(VX_VARIABLE_FAULT_CODE); }}
			// 0x71 // fast but also query in case only remaining switch time is configured
			if ((this->switch_active_binary_sensor_ != nullptr) || (this->switch_remaining_sensor_ != nullptr)) { requestVariable(VX_VARIABLE_FLAGS_06); }
			// 0x79 // fast but only if special function is active
			if (this->switch_remaining_sensor_ != nullptr) { 
				if (buffer.contains(VX_VARIABLE_FLAGS_06)) {
					if ((buffer[VX_VARIABLE_FLAGS_06] & VX_06_FIREPLACE_FLAG_IS_ACTIVE) != 0x00 ) { requestVariable(VX_VARIABLE_SWITCH_REMAINING); }
				}
			}
			// 0xA3 // fast query // requied by climate sensor
			requestVariable(VX_VARIABLE_STATUS);
			// 0xA4
			requestVariable(VX_VARIABLE_HEATING_TARGET);
			// 0xA5 // slow query
			if (this->fan_speed_max_number_        != nullptr) { if ((!this->fan_speed_max_number_->has_state()) || retrySlow )       { requestVariable(VX_VARIABLE_FAN_SPEED_MAX); }}
			// 0xA9 // slow query // if either sensor is configured
			retryFlag = 0;
			if (this->fan_speed_min_number_        != nullptr) { if ((!this->fan_speed_min_number_->has_state())     || retrySlow ) { retryFlag = 1; }}
			if (this->fan_speed_default_sensor_    != nullptr) { if ((!this->fan_speed_default_sensor_->has_state()) || retrySlow ) { retryFlag = 1; }}
			if (retryFlag)     { requestVariable(VX_VARIABLE_FAN_SPEED_MIN); }
			// 0xA6 // slow query // required also for service reset
			if ((this->service_reset_button_ != nullptr) || (this->service_period_sensor_ != nullptr)) {
				if (!buffer.contains(VX_VARIABLE_SERVICE_PERIOD) || retrySlow ) { requestVariable(VX_VARIABLE_SERVICE_PERIOD   ); }
			}
			// 0xAA
			retryFlag = 0;
			if (this->switch_type_text_sensor_     != nullptr) { if ((!this->switch_type_text_sensor_->has_state())   || retrySlow ) { retryFlag = 1; }}
			if (this->switch_type_select_select_   != nullptr) { if ((!this->switch_type_select_select_->has_state()) || retrySlow ) { retryFlag = 1; }}
			if (retryFlag) { requestVariable(VX_VARIABLE_PROGRAM); }
			// 0xAB
			if (this->service_remaining_sensor_    != nullptr) { if ((!this->service_remaining_sensor_->has_state())  || retrySlow ) { requestVariable(VX_VARIABLE_SERVICE_REMAINING); }}
			// 0xAF
			if (this->heat_bypass_number_          != nullptr) { if ((!this->heat_bypass_number_->has_state())        || retrySlow ) { requestVariable(VX_VARIABLE_T_HEAT_BYPASS); }}
		}



		void ValloxVentilation::loop() {
			ionow = millis();

			// Check incoming data, read unsigned char
			if (this->available() >= 1) {
				this->read_byte(&iomessage_recv_byte);
				iots = ionow;
				updateState(true); // received new byte
			} else {
				if (getState() == RECEIVED_IDLE) {  // got time on our hands and are not waiting on something like checksum from previous sent message, check if we have something new to send.
					if ((ionow - iosendts) > SEND_INTERVAL) {  // non-blocking delay between sending packets
						if (iomessage_send_command_queue.size() > 0) {  // send_command_queue only receives packets with required checksum
							sendMessage(iomessage_send_command_queue.front());
							iots = ionow;
							iosendts = ionow;
							setState(SENT_PACKET_AWAITING_CHECKSUM);
						} else if (iomessage_send_queue.size() > 0) {
							sendMessage(iomessage_send_queue.front());
							iots = ionow;
							iosendts = ionow;
							// broadcast or variable request, no need to wait for checksum
							iomessage_send_queue.pop();
							setState(RECEIVED_IDLE);
						}
					}
				}
				if ( (ionow - iots) > VX_REPLY_WAIT_TIME ) { updateState(false); }  // no input/output during last 10ms, update state, either check for retry on send or revert back to RECEIVED_IDLE
			} 
			if ( (ionow - iofastqueryts) > FAST_QUERY_INTERVAL) {     // check if some of the enabled variables are not filled yet, send query again
				retryVariables();
			}
		}


		void ValloxVentilation::setState(uint newstate) {
			iostate = newstate;
			if (newstate == RECEIVED_IDLE) {
				clearChecksum();
			}
		}

		uint ValloxVentilation::getState() {
			return iostate;
		}

		void ValloxVentilation::clearChecksum() {
			iochecksum = 0x00;
		}

		void ValloxVentilation::addChecksum(unsigned char newbyte) {
			iochecksum += newbyte;
		}
		unsigned char ValloxVentilation::getChecksum() {
			return iochecksum;
		}

        unsigned char ValloxVentilation::getChecksum(vector<unsigned char> newmessage) {
			assert(newmessage.size() >= (VX_MSG_LENGTH-1));
			return newmessage[0]+newmessage[1]+newmessage[2]+newmessage[3]+newmessage[4];
		}

		void ValloxVentilation::sendMessage(vector<unsigned char> newmessage) {
			unsigned char message[VX_MSG_LENGTH];

			message[0] = newmessage[0];
			message[1] = newmessage[1];
			message[2] = newmessage[2];
			message[3] = newmessage[3];
			message[4] = newmessage[4];
			message[5] = newmessage[5];
			this->write_array(message, VX_MSG_LENGTH);
		}

		void ValloxVentilation::updateState(bool ionewdata) {

			// Timeout receiving data
			if ((ionow - iosendts) > VX_REPLY_WAIT_TIME) {
				if (getState() == SENT_PACKET_AWAITING_CHECKSUM) {
					if (iomessage_send_retries >= VX_MAX_RETRIES) {
						if (iomessage_send_command_queue.size() > 0) { iomessage_send_command_queue.pop(); }  ////// give up, discard message ///////
						setState(RECEIVED_IDLE);
						ESP_LOGD(TAG,"failed to receive checksum confirmation on sent command, giving up");
					} else {
						iomessage_send_retries++;
						iosendts = ionow;
						sendMessage(iomessage_send_command_queue.front());
						ESP_LOGD(TAG,"failed to receive checksum confirmation on sent command, retrying");
					}
				} else if ((ionow - iots) > 100) {
					setState(RECEIVED_IDLE);  // if during receive state, nothing received for 100ms, go back to IDLE and evaluate below based on fresh packet
				}
			}
			if (ionewdata) {
				switch (getState())
				{
					case RECEIVED_IDLE:
						if (iomessage_recv_byte == 0x01) {
							iomessage_recv[0] = iomessage_recv_byte;
							addChecksum(iomessage_recv_byte);
							setState(RECEIVED_SYSTEM_AWAITING_SENDER);
						}
						break;
					case RECEIVED_SYSTEM_AWAITING_SENDER:
						if (iomessage_recv_byte > 0x2f || iomessage_recv_byte < 0x11 || iomessage_recv_byte == 0x20) { // invalid SENDER addressess
							if (iomessage_recv_byte == 0x01) {
								iomessage_recv[0] = iomessage_recv_byte;  // actually not needed as first byte is always 0x01 
								setState(RECEIVED_SYSTEM_AWAITING_SENDER);  // let's retry from here including the received system variable 0x01
								clearChecksum();
								addChecksum(0x01);
							} else {
								setState(RECEIVED_IDLE); // nothing valid and not the start of new packet.. ignore and start from scratch
							}
						} else {
							iomessage_recv[1] = iomessage_recv_byte;
							addChecksum(iomessage_recv_byte);
							setState(RECEIVED_SENDER_AWAITING_RECIPIENT);
						}
						break;
					case RECEIVED_SENDER_AWAITING_RECIPIENT:
						if (iomessage_recv_byte > 0x2f || iomessage_recv_byte < 0x10)  {
							setState(RECEIVED_IDLE); // invalid RECIPIENT addressess
							if (iomessage_recv_byte == 0x01) {
								iomessage_recv[0] = iomessage_recv_byte; // actually not needed as first byte is always 0x01 and has already been assigned to get here
								setState(RECEIVED_SYSTEM_AWAITING_SENDER);  // let's retry from here including the received system variable 0x01
								clearChecksum();
								addChecksum(0x01);
							} else {
								setState(RECEIVED_IDLE); // nothing valid and not the start of new packet.. ignore and start from scratch
							}
						} else {
							iomessage_recv[2] = iomessage_recv_byte;
							addChecksum(iomessage_recv_byte);
							setState(RECEIVED_RECIPIENT_AWAITING_VARIABLE);
						}
						break;
					case RECEIVED_RECIPIENT_AWAITING_VARIABLE:
						iomessage_recv[3] = iomessage_recv_byte;
						addChecksum(iomessage_recv_byte);
						setState(RECEIVED_VARIABLE_AWAITING_DATA);
						break;
					case RECEIVED_VARIABLE_AWAITING_DATA:
						iomessage_recv[4] = iomessage_recv_byte;
						addChecksum(iomessage_recv_byte);
						setState(RECEIVED_DATA_AWAITING_CHECKSUM);
						break;
					case RECEIVED_DATA_AWAITING_CHECKSUM:
						if (iomessage_recv_byte == getChecksum()) { decodeMessage(iomessage_recv); }  // fall back to RECEIVED_IDLE on invalid checksum but only decode if valid
						setState(RECEIVED_IDLE);
						break;
					case SENT_PACKET_AWAITING_CHECKSUM:
						if (iomessage_send_command_queue.size() > 0) {
							if (iomessage_recv_byte == iomessage_send_command_queue.front()[5]) {
								iomessage_send_retries = 0;
								if (iomessage_send_command_queue.size() > 0) { iomessage_send_command_queue.pop(); }
								setState(RECEIVED_IDLE);    // all ok, received checksum, go back to idle
							} // received something but not matching checksum, stay in current state and wait for 10ms to expire
						} else {  // should not happen, just to be safe..
							setState(RECEIVED_IDLE);
						}
						break;
					default:
						setState(RECEIVED_IDLE);
						break;
				}
			}
		}



		void ValloxVentilation::requestVariable(unsigned char variable) {
			vector<unsigned char> newmessage(6);
			newmessage[0] = VX_MSG_DOMAIN;
			newmessage[1] = VX_MSG_THIS_PANEL;
			newmessage[2] = VX_MSG_MAINBOARD_1;
			newmessage[3] = VX_MSG_POLL_BYTE;
			newmessage[4] = variable;
			newmessage[5] = getChecksum(newmessage);

			if (iomessage_send_queue.size() < SEND_QUEUE_MAX_DEPTH) { iomessage_send_queue.push(newmessage); } // drop new requests if queue too full
		}

		void ValloxVentilation::setVariable(unsigned char variable, unsigned char value) { // no target, send to mainboard_1 and all panels.  // maybe check for status variable and only send to mainboard?
			setVariable(variable, value, VX_MSG_PANELS);
			// setVariable(variable, value, VX_MSG_MAINBOARDS);  // TODO: disable for now. This can be enabled if needed but keep in mind that we would need to ensure that the sending order is maintained (direct commands will get prioritized over broadcasts thus a broadcast of a previous command could arrive after a direct command that was issued later)
			setVariable(variable, value, VX_MSG_MAINBOARD_1);
		}

		void ValloxVentilation::setVariable(unsigned char variable, unsigned char value, unsigned char target) {
			vector<unsigned char> newmessage(6);
			newmessage[0] = VX_MSG_DOMAIN;
			newmessage[1] = VX_MSG_THIS_PANEL;
			newmessage[2] = target;
			newmessage[3] = variable;
			newmessage[4] = value;
			newmessage[5] = getChecksum(newmessage);

			if (newmessage[2] == VX_MSG_MAINBOARDS || newmessage[2] == VX_MSG_PANELS) {  // put broadcast messages into the non-ack queue
				if (iomessage_send_queue.size() < SEND_QUEUE_MAX_DEPTH) { iomessage_send_queue.push(newmessage); }
			} else {
				if (iomessage_send_command_queue.size() < SEND_QUEUE_MAX_DEPTH) { iomessage_send_command_queue.push(newmessage); }
			}
		}

		void ValloxVentilation::decodeFlags(unsigned char element, unsigned char value) {
			if (element == VX_VARIABLE_STATUS) {
				if (this->status_on_binary_sensor_      != nullptr) { this->status_on_binary_sensor_->publish_state(      (value & VX_STATUS_FLAG_POWER)        != 0x00 ); }
				if (this->service_needed_binary_sensor_ != nullptr) { this->service_needed_binary_sensor_->publish_state( (value & VX_STATUS_FLAG_SERVICE)      != 0x00 ); }
				if (this->heating_binary_sensor_        != nullptr) { this->heating_binary_sensor_->publish_state(        (value & VX_STATUS_FLAG_HEATING)      != 0x00 ); }
				if (this->heating_mode_binary_sensor_   != nullptr) { this->heating_mode_binary_sensor_->publish_state(   (value & VX_STATUS_FLAG_HEATING_MODE) != 0x00 ); }
				if (this->problem_binary_sensor_        != nullptr) { this->problem_binary_sensor_->publish_state(        (value & VX_STATUS_FLAG_FAULT)        != 0x00 ); }
				if ((value & VX_STATUS_FLAG_POWER) != 0x00) {
					this->mode   = ( (value & VX_STATUS_FLAG_HEATING_MODE) != 0x00 ) ? climate::CLIMATE_MODE_HEAT      : climate::CLIMATE_MODE_FAN_ONLY ;
					this->action = ( (value & VX_STATUS_FLAG_HEATING_MODE) != 0x00 ) ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_FAN ;
				} else {
					this->mode   = climate::CLIMATE_MODE_OFF;
					this->action = climate::CLIMATE_ACTION_OFF;
				}
				this->publish_state();
			} else if (element == VX_VARIABLE_IO_08) {
				if (this->summer_mode_binary_sensor_      != nullptr) { this->summer_mode_binary_sensor_->publish_state(       (value & VX_08_FLAG_SUMMER_MODE)   != 0x00 ) ; }
				if (this->status_motor_in_binary_sensor_  != nullptr) { this->status_motor_in_binary_sensor_->publish_state( !((value & VX_08_FLAG_MOTOR_IN)      != 0x00 )); }
				if (this->status_motor_out_binary_sensor_ != nullptr) { this->status_motor_out_binary_sensor_->publish_state(!((value & VX_08_FLAG_MOTOR_OUT)     != 0x00 )); }
				if (this->front_heating_binary_sensor_    != nullptr) { this->front_heating_binary_sensor_->publish_state(     (value & VX_08_FLAG_FRONT_HEATING) != 0x00 ) ; }
				if (this->error_relay_binary_sensor_      != nullptr) { this->error_relay_binary_sensor_->publish_state(       (value & VX_08_FLAG_ERROR_RELAY)   != 0x00 ) ; }
			} else if (element == VX_VARIABLE_FLAGS_06) {
				if (this->switch_active_binary_sensor_ != nullptr) { this->switch_active_binary_sensor_->publish_state( (value & VX_06_FIREPLACE_FLAG_IS_ACTIVE) != 0x00 ); }
			} else if (element == VX_VARIABLE_PROGRAM) {
				if (this->switch_type_text_sensor_ != nullptr) {
					this->switch_type_text_sensor_->publish_state( ((value & VX_PROGRAM_SWITCH_TYPE) != 0x00) ? SWITCH_TYPE_ACTION_BOOST : SWITCH_TYPE_ACTION_FIREPLACE );
				}
				if (this->switch_type_select_select_ != nullptr) {
					this->switch_type_select_select_->publish_state( ((value & VX_PROGRAM_SWITCH_TYPE) != 0x00) ? SWITCH_TYPE_ACTION_BOOST : SWITCH_TYPE_ACTION_FIREPLACE );
				}
			}
		}
		void ValloxVentilation::setFlags(unsigned char element, unsigned char bitpos, bool value) {
			if (value) {
				setVariable(element, buffer[element] | bitpos); // set bit to 1
			} else {
				setVariable(element, buffer[element] & (~bitpos)); // set bit to 0
			}
		}


		void ValloxVentilation::decodeMessage(const unsigned char message[]) {
			unsigned char variable = message[3];
			unsigned char value = message[4];
			int val = NOT_SET;
			unsigned long now = millis();

//				if ((message[2] == VX_MSG_THIS_PANEL || message[2] == VX_MSG_PANELS) && message[1] != VX_MSG_THIS_PANEL) {
			if (message[2] == VX_MSG_THIS_PANEL || message[2] == VX_MSG_PANELS) {
				// Temperature
				if (variable == VX_VARIABLE_T_OUTSIDE)  {
					if ( buffer[VX_VARIABLE_T_OUTSIDE] == value ) {
						if (this->temperature_outside_sensor_  != nullptr) { this->temperature_outside_sensor_->publish_state(convNtc2Cel(value));  }
					} // only update on two subsequent identical values
					buffer[VX_VARIABLE_T_OUTSIDE] = value;
				} else if (variable == VX_VARIABLE_T_OUTGOING) {
					if ( buffer[VX_VARIABLE_T_OUTGOING] == value ) {
						if (this->temperature_outgoing_sensor_ != nullptr) { this->temperature_outgoing_sensor_->publish_state(convNtc2Cel(value)); }
					}
					buffer[VX_VARIABLE_T_OUTGOING] = value;
				} else if (variable == VX_VARIABLE_T_INSIDE)   {
					if ( buffer[VX_VARIABLE_T_INSIDE] == value ) {
						if (this->temperature_inside_sensor_   != nullptr) { this->temperature_inside_sensor_->publish_state(convNtc2Cel(value));   }
						// Inside temperature used for climate current temperature
						this->current_temperature = convNtc2Cel(value);
						this->publish_state();
					}
					buffer[VX_VARIABLE_T_INSIDE] = value;
				} else if (variable == VX_VARIABLE_T_INCOMING) {
					if ( buffer[VX_VARIABLE_T_INCOMING] == value ) {
						if (this->temperature_incoming_sensor_ != nullptr) { this->temperature_incoming_sensor_->publish_state(convNtc2Cel(value)); }
					}
					buffer[VX_VARIABLE_T_INCOMING] = value;
				} else if (variable == VX_VARIABLE_RH1) {  // RH 1
					buffer[VX_VARIABLE_RH1] = value;
					val = convHex2Rh(value);
					if (val!=NOT_SET) {
						if (this->humidity_1_sensor_ != nullptr) { this->humidity_1_sensor_->publish_state(val); }
					}
				} else if (variable == VX_VARIABLE_RH2) {  // RH 2
					buffer[VX_VARIABLE_RH2] = value;
					val =  convHex2Rh(value);
					if (val!=NOT_SET) {
						if (this->humidity_2_sensor_ != nullptr) { this->humidity_2_sensor_->publish_state(val); }
					}
				} else if (variable == VX_VARIABLE_CO2_HI) {       // CO2, Let's assume that the timeinterval for the same value is something pre-defined..
					buffer[VX_VARIABLE_CO2_HI] = value;
					if (this->co2_sensor_ != nullptr) {
						buffer_co2_hi_ts = millis();
						if ((buffer[VX_VARIABLE_CO2_LO] != 0x00) || (buffer[VX_VARIABLE_CO2_HI] != 0x00)) {   // only publish if not zero
							if (buffer_co2_lo_ts > millis() - CO2_LIFE_TIME_MS) {  // only publish if corresponding lo value is within defined lifetime
								this->co2_sensor_->publish_state(buffer[VX_VARIABLE_CO2_LO] + (buffer[VX_VARIABLE_CO2_HI] << 8));
							}
						}
					}
				} else if (variable == VX_VARIABLE_CO2_LO) {
					buffer[VX_VARIABLE_CO2_LO] = value;
					if (this->co2_sensor_ != nullptr) {
						buffer_co2_lo_ts = millis();
						if ((buffer[VX_VARIABLE_CO2_LO] != 0x00) || (buffer[VX_VARIABLE_CO2_HI] != 0x00)) {   // only publish if not all zero
							if (buffer_co2_hi_ts > millis() - CO2_LIFE_TIME_MS) {  // only publish if corresponding hi value is within defined lifetime
								this->co2_sensor_->publish_state(buffer[VX_VARIABLE_CO2_LO] + (buffer[VX_VARIABLE_CO2_HI] << 8));
							}
						}
					}
				} else if (variable == VX_VARIABLE_FAN_SPEED) {        // Others (config object)
					buffer[VX_VARIABLE_FAN_SPEED] = value;
					val = convHex2FanSpeed(value);
					if (val!=NOT_SET) {
						if (this->fan_speed_sensor_ != nullptr) { this->fan_speed_sensor_->publish_state(val); }
						this->custom_fan_mode = (optional<std::string>) to_string(val); // also set fan mode corresponding to fan speed
						this->publish_state();
					}
				} else if (variable == VX_VARIABLE_FAN_SPEED_MIN) {
					buffer[VX_VARIABLE_FAN_SPEED_MIN] = value;
					val = convHex2FanSpeed(value);
					if (val!=NOT_SET) {
						if (this->fan_speed_default_sensor_ != nullptr) { this->fan_speed_default_sensor_->publish_state(val); } // default fan speed is minimum fan speed, maybe this sensor is superfluous
						if (this->fan_speed_min_number_ != nullptr) { this->fan_speed_min_number_->publish_state(val); }
					}
				} else if (variable == VX_VARIABLE_STATUS) {
					buffer[VX_VARIABLE_STATUS] = value;
					decodeFlags(VX_VARIABLE_STATUS, value);
				} else if (variable == VX_VARIABLE_IO_08) {
					buffer[VX_VARIABLE_IO_08] = value;
					decodeFlags(VX_VARIABLE_IO_08, value);
				} else if (variable == VX_VARIABLE_FLAGS_06) {
					buffer[VX_VARIABLE_FLAGS_06] = value;
					decodeFlags(VX_VARIABLE_FLAGS_06, value);
				} else if (variable == VX_VARIABLE_SERVICE_PERIOD) {
					buffer[VX_VARIABLE_SERVICE_PERIOD] = value;
					service_period = value;
					val = (float)value;
					if (this->service_period_sensor_ != nullptr) { this->service_period_sensor_->publish_state(val); }
				} else if (variable == VX_VARIABLE_SERVICE_REMAINING) {
					buffer[VX_VARIABLE_SERVICE_REMAINING] = value;
					val = (float)value;
					if (this->service_remaining_sensor_ != nullptr) { this->service_remaining_sensor_->publish_state(val); }
				} else if (variable == VX_VARIABLE_SWITCH_REMAINING) {
					buffer[VX_VARIABLE_SWITCH_REMAINING] = value;
					val = (float)value;
					if (this->switch_remaining_sensor_ != nullptr) { this->switch_remaining_sensor_->publish_state(val); }
				} else if (variable == VX_VARIABLE_HEATING_TARGET) {
					buffer[VX_VARIABLE_HEATING_TARGET] = value;
					val = convNtc2Cel(value);
					if (this->temperature_target_sensor_  != nullptr) {
						this->temperature_target_sensor_->publish_state(val);
						this->target_temperature = val;
						this->publish_state();
					}
				} else if (variable == VX_VARIABLE_PROGRAM) {
					buffer[VX_VARIABLE_PROGRAM] = value;
					decodeFlags(VX_VARIABLE_PROGRAM, value);
				} else if (variable == VX_VARIABLE_T_HEAT_BYPASS) { // Heat Bypass temperature
					buffer[VX_VARIABLE_T_HEAT_BYPASS] = value;
					if (this->heat_bypass_number_ != nullptr) { this->heat_bypass_number_->publish_state(convNtc2Cel(value)); }
				} else if (variable == VX_VARIABLE_FAULT_CODE) {
					buffer[VX_VARIABLE_FAULT_CODE] = value;
					if (this->fault_condition_text_sensor_ != nullptr) {
						if (value == 0x05) { this->fault_condition_text_sensor_->publish_state("incoming air sensor fault");         }
						if (value == 0x06) { this->fault_condition_text_sensor_->publish_state("carbon dioxide alarm");              }
						if (value == 0x07) { this->fault_condition_text_sensor_->publish_state("outside sensor fault");              }
						if (value == 0x08) { this->fault_condition_text_sensor_->publish_state("inside air sensor fault");           }
						if (value == 0x09) { this->fault_condition_text_sensor_->publish_state("danger of the water coil freezing"); }
						if (value == 0x0a) { this->fault_condition_text_sensor_->publish_state("outgoing air sensor fault");         }
					}
				} else if (variable == VX_VARIABLE_FAN_SPEED_MAX) {
					buffer[VX_VARIABLE_FAN_SPEED_MAX] = value;
					val = convHex2FanSpeed(value);
					if (val!=NOT_SET) {
						if (this->fan_speed_max_number_ != nullptr) { this->fan_speed_max_number_->publish_state(val); }
					}
				} else {
			 // variable not recognized
				}
			}
		}



		//////////////////////////////////
		// Helper functions //
		/////////////////////////////
		unsigned char ValloxVentilation::convCel2Ntc(int cel) {
			for (int i = 0; i < 256; i++) {
				if (vxTemps[i] == cel) {
					return i;
				}
			}
			// we should not be here, return 10 Cel as default
			return 0x83;
		}

		int ValloxVentilation::convNtc2Cel(unsigned char ntc) {
			int i = (uint8_t)ntc;
			return vxTemps[i];
		}

		int ValloxVentilation::convHex2Rh(unsigned char hex) {
			if (hex >= 51) {
				return (hex - 51) / 2.04;
			} else {
				return NOT_SET;
			}
		}

		unsigned char ValloxVentilation::convFanSpeed2Hex(int fan) {
			if (fan > 0 && fan < 9) {
				return vxFanSpeeds[fan - 1];
			}
			// we should not be here, return speed 1 as default
			return VX_FAN_SPEED_1;
		}

		int ValloxVentilation::convHex2FanSpeed(unsigned char hex) {
			for (int i = 0; i < sizeof(vxFanSpeeds); i++) {
				if (vxFanSpeeds[i] == hex) {
					return i + 1;
				}
			}
			return NOT_SET;
		}


	}
}
