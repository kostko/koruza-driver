/*
 * koruza-driver - KORUZA driver
 *
 * Copyright (C) 2016 Jernej Kos <jernej@kos.mx>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KORUZA_MESSAGE_H
#define KORUZA_MESSAGE_H

#include <stdint.h>
#include <sys/types.h>

// Maximum number of TLVs inside a message.
#define MAX_TLV_COUNT 25

/**
 * TLVs supported by the protocol.
 */
typedef enum {
  TLV_COMMAND = 1,
  TLV_REPLY = 2,
  TLV_CHECKSUM = 3,
  TLV_MOTOR_POSITION = 4,
  TLV_CURRENT_READING = 5,
  TLV_SFP_CALIBRATION = 6,
  TLV_ERROR_REPORT = 7,
  TLV_POWER_READING = 8,
  TLV_ENCODER_VALUE = 9,
  TLV_VIBRATION_VALUE = 10,

  // Network communication TLVs.
  TLV_NET_HELLO = 100,
  TLV_NET_SIGNATURE = 101,
} tlv_type_t;

/**
 * Commands supported by the command TLV.
 */
typedef enum {
  COMMAND_GET_STATUS = 1,
  COMMAND_MOVE_MOTOR = 2,
  COMMAND_SEND_IR = 3,
  COMMAND_REBOOT = 4,
  COMMAND_FIRMWARE_UPGRADE = 5,
  COMMAND_HOMING = 6,
  COMMAND_RESTORE_MOTOR = 7,
} tlv_command_t;

/**
 * Replies supported by the reply TLV.
 */
typedef enum {
  REPLY_STATUS_REPORT = 1,
  REPLY_ERROR_REPORT = 2,
} tlv_reply_t;

/**
 * Contents of the motor position TLV.
 */
typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} tlv_motor_position_t;

/**
 * Contents of the encoder value TLV.
 */
typedef struct {
  int32_t x;
  int32_t y;
} tlv_encoder_value_t;

/**
 * Contents of the accelerometer value TLV.
 */
typedef struct {
  // Average values.
  int32_t avg_x[4];
  int32_t avg_y[4];
  int32_t avg_z[4];

  // Maximum values.
  int32_t max_x[4];
  int32_t max_y[4];
  int32_t max_z[4];
} tlv_vibration_value_t;

/**
 * Contents of the error report TLV.
 */
typedef struct {
  uint32_t code;
} tlv_error_report_t;

/**
 * Contents of the motor position TLV.
 */
typedef struct {
  uint32_t offset_x;
  uint32_t offset_y;
} tlv_sfp_calibration_t;

/**
 * Message operations result codes.
 */
typedef enum {
  MESSAGE_SUCCESS = 0,
  MESSAGE_ERROR_TOO_MANY_TLVS = -1,
  MESSAGE_ERROR_OUT_OF_MEMORY = -2,
  MESSAGE_ERROR_BUFFER_TOO_SMALL = -3,
  MESSAGE_ERROR_PARSE_ERROR = -4,
  MESSAGE_ERROR_CHECKSUM_MISMATCH = -5,
  MESSAGE_ERROR_TLV_NOT_FOUND = -6
} message_result_t;

/**
 * Representation of a TLV.
 */
typedef struct {
  uint8_t type;
  uint16_t length;
  uint8_t *value;
} tlv_t;

/**
 * Representation of a protocol message.
 */
typedef struct {
  size_t length;
  tlv_t tlv[MAX_TLV_COUNT];
} message_t;

/**
 * Initializes a protocol message. This function should be called on any newly
 * created message to ensure that the underlying memory is properly initialized.
 *
 * @param message Message instance to initialize
 * @return Operation result code
 */
message_result_t message_init(message_t *message);

/**
 * Frees a protocol message.
 *
 * @param message Message instance to free
 */
void message_free(message_t *message);

/**
 * Parses a protocol message.
 *
 * @param message Destination message instance to parse into
 * @param data Raw data to parse
 * @param length Size of the data buffer
 * @return Operation result code
 */
message_result_t message_parse(message_t *message, const uint8_t *data, size_t length);

/**
 * Adds a raw TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param type TLV type
 * @param length TLV length
 * @param value TLV value
 * @return Operation result code
 */
message_result_t message_tlv_add(message_t *message, uint8_t type, uint16_t length, const uint8_t *value);

/**
 * Adds a command TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param command Command argument
 * @return Operation result code
 */
message_result_t message_tlv_add_command(message_t *message, tlv_command_t command);

/**
 * Adds a reply TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param reply Reply argument
 * @return Operation result code
 */
message_result_t message_tlv_add_reply(message_t *message, tlv_reply_t reply);

/**
 * Adds a motor position TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param position Motor position structure
 * @return Operation result code
 */
message_result_t message_tlv_add_motor_position(message_t *message, const tlv_motor_position_t *position);

/**
 * Adds an error report TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param report Error report structure
 * @return Operation result code
 */
message_result_t message_tlv_add_error_report(message_t *message, const tlv_error_report_t *report);

/**
 * Adds a current reading TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param current Current reading
 * @return Operation result code
 */
message_result_t message_tlv_add_current_reading(message_t *message, uint16_t current);

/**
 * Adds a power reading TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param power Power reading
 * @return Operation result code
 */
message_result_t message_tlv_add_power_reading(message_t *message, uint16_t power);

/**
 * Adds an encoder value TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param value Encoder value
 * @return Operation result code
 */
message_result_t message_tlv_add_encoder_value(message_t *message, const tlv_encoder_value_t *value);

/**
 * Adds an vibration value TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param value Vibration value
 * @return Operation result code
 */
message_result_t message_tlv_add_vibration_value(message_t *message, const tlv_vibration_value_t *value);

/**
 * Adds a SFP calibration TLV to a protocol message.
 *
 * @param message Destination message instance to add the TLV to
 * @param position SFP calibration structure
 * @return Operation result code
 */
message_result_t message_tlv_add_sfp_calibration(message_t *message, const tlv_sfp_calibration_t *calibration);

/**
 * Adds a checksum TLV to a protocol message. The checksum value is automatically
 * computed over all the TLVs currently contained in the message.
 *
 * @param message Destination message instance to add the TLV to
 * @return Operation result code
 */
message_result_t message_tlv_add_checksum(message_t *message);

/**
 * Find the first TLV of a given type in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param type Type of TLV that should be returned
 * @param destination Destination buffer
 * @param length Length of the destination buffer
 * @return Operation result code
 */
message_result_t message_tlv_get(const message_t *message, uint8_t type, uint8_t *destination, size_t length);

/**
 * Find the first command TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param command Destination command variable
 * @return Operation result code
 */
message_result_t message_tlv_get_command(const message_t *message, tlv_command_t *command);

/**
 * Find the first reply TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param reply Destination reply variable
 * @return Operation result code
 */
message_result_t message_tlv_get_reply(const message_t *message, tlv_reply_t *reply);

/**
 * Find the first motor position TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param position Destination position variable
 * @return Operation result code
 */
message_result_t message_tlv_get_motor_position(const message_t *message, tlv_motor_position_t *position);

/**
 * Find the first error report TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param report Destination error report variable
 * @return Operation result code
 */
message_result_t message_tlv_get_error_report(const message_t *message, tlv_error_report_t *report);

/**
 * Find the first current reading TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param current Destination current variable
 * @return Operation result code
 */
message_result_t message_tlv_get_current_reading(const message_t *message, uint16_t *current);

/**
 * Find the first encoder value TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param value Destination encoder value variable
 * @return Operation result code
 */
message_result_t message_tlv_get_encoder_value(const message_t *message, tlv_encoder_value_t *value);

/**
 * Find the first vibration value TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param value Destination vibration value variable
 * @return Operation result code
 */
message_result_t message_tlv_get_vibration_value(const message_t *message, tlv_vibration_value_t *value);

/**
 * Find the first SFP calibration TLV in a message and copies it.
 *
 * @param message Message instance to get the TLV from
 * @param calibration Destination calibration variable
 * @return Operation result code
 */
message_result_t message_tlv_get_sfp_calibration(const message_t *message, tlv_sfp_calibration_t *calibration);

/**
 * Returns the size a message would take in its serialized form.
 *
 * @param message Message instance to calculate the size for
 */
size_t message_serialized_size(const message_t *message);

/**
 * Serializes a protocol message into a destination buffer.
 *
 * @param buffer Destination buffer
 * @param length Length of the destination buffer
 * @param message Message instance to serialize
 * @return Number of bytes written serialized to the buffer or error code
 */
ssize_t message_serialize(uint8_t *buffer, size_t length, const message_t *message);

/**
 * Prints a debug representation of a protocol message.
 *
 * @param message Protocol message to print
 */
void message_print(const message_t *message);

#endif
