// Copyright 2020
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>

#include "ros2_ouster/OS1/OS1_sensor.hpp"
#include "ros2_ouster/conversions.hpp"
#include "ros2_ouster/exception.hpp"
#include "ros2_ouster/interfaces/metadata.hpp"

namespace OS1
{

OS1Sensor::OS1Sensor()
: SensorInterface()
{
  _lidar_packet.resize(lidar_packet_bytes + 1);
  _imu_packet.resize(imu_packet_bytes + 1);
}

void OS1Sensor::reset(const ros2_ouster::Configuration & config)
{
  _ouster_client.reset();
  configure(config);
}

void OS1Sensor::configure(const ros2_ouster::Configuration & config)
{
  if (!OS1::lidar_mode_of_string(config.lidar_mode)) {
    throw ros2_ouster::OusterDriverException(
            std::string("Invalid lidar mode %s!", config.lidar_mode.c_str()));
    exit(-1);
  }

  _ouster_client = OS1::init_client(
    config.lidar_ip, config.computer_ip,
    OS1::lidar_mode_of_string(config.lidar_mode),
    config.lidar_port, config.imu_port);

  if (!_ouster_client) {
    throw ros2_ouster::OusterDriverException(
            std::string("Failed to create connection to lidar."));
  }
}

ros2_ouster::ClientState OS1Sensor::get()
{
  const ClientState state = OS1::poll_client(*_ouster_client);

  if (state != ClientState::LIDAR_DATA && state != ClientState::IMU_DATA) {
    throw ros2_ouster::OusterDriverException(
            std::string("Failed to get valid sensor data "
            "information from lidar, returned state %s.",
            ros2_ouster::toString(state).c_str()));
  }
  return state;
}

uint8_t * OS1Sensor::readPacket(const ros2_ouster::ClientState & state)
{
  switch (state) {
    case ClientState::IMU_DATA:
      if (read_imu_packet(*_ouster_client, _lidar_packet.data())) {
        return _lidar_packet.data();
      } else {
        return nullptr;
      }
    case ClientState::LIDAR_DATA:
      if (read_lidar_packet(*_ouster_client, _imu_packet.data())) {
        return _imu_packet.data();
      } else {
        return nullptr;
      }
  }
}

ros2_ouster::Metadata OS1Sensor::getMetadata()
{
  if (_ouster_client) {
    return OS1::parse_metadata(OS1::get_metadata(*_ouster_client));
  } else {
    return {"UNKNOWN", "UNKNOWN", "UNNKOWN", "UNNKOWN",
      {}, {}, {}, {}, 7503, 7502};
  }
}

}  // namespace OS1