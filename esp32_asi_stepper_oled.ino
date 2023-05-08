#include <utility>
#include <Bounce2.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include "display.h"
#include "stepper.h"

const auto MS_TO_KTS = 1.94384f;

auto wm_reset = Bounce();
auto display = Display();
auto asi_motor = Stepper(26, 25, 33, 32, 19);

enum class State {
  NoNetwork,
  NoCondor,
  Running,
  Calibration
};

auto state = State::NoNetwork;
WiFiManager wm;
auto udp = WiFiUDP();
#define UDP_TX_PACKET_MAX_SIZE 2048
char udp_buffer[UDP_TX_PACKET_MAX_SIZE];

void setup() {
  Serial.begin(9600);
  Serial.println("Hello, world");

  display.setup();
  display.print_msg("Not connected...");

  WiFi.mode(WIFI_STA);
  wm.autoConnect("Condor ASI Demo");

  udp.begin(55278);
  wm_reset.attach(12, INPUT_PULLUP);
  wm_reset.interval(5);
}

int kts_to_step(float airspeed_kts) {
  static const std::pair<float, int> mapping[] = {
    { 0, 30 },
    { 15, 30 },
    { 20, 310 },
    { 30, 825 },
    { 40, 1360 },
    { 50, 1930 },
    { 60, 2485 },
    { 70, 2950 },
    { 80, 3420 },
    { 90, 3740 },
    { 100, 4100 },
    { 110, 4500 },
    { 120, 4800 },
    { 130, 5070 },
    { 140, 5340 },
    { 150, 5580 },
    { 160, 5820 }
  };
  auto prev = mapping[0];
  if (airspeed_kts < prev.first) {
    return prev.second;
  }
  for (const auto point : mapping) {
    if (prev.first <= airspeed_kts && airspeed_kts < point.first) {
      const auto ratio = (airspeed_kts - prev.first) / (point.first - prev.first);
      return prev.second + (point.second - prev.second) * ratio;
    }
    prev = point;
  }
  return prev.second;
}

void check_wm_reset() {
  wm_reset.update();
  if (wm_reset.fell()) {
    display.print_msg("Wifi reset");
    wm.resetSettings();
  }
}

State loop_no_network() {
  check_wm_reset();
  if (WiFi.isConnected()) {
    Serial.println("Wifi connected");
    Serial.println(WiFi.localIP().toString());

    display.print_msg("Wifi on");
    delay(500);
    auto ip_addr = WiFi.localIP().toString();
    display.print_msg(ip_addr.substring(0, 9), 0);
    display.print_msg(ip_addr.substring(9, ip_addr.length()), 1);
    return State::NoCondor;
  }
  return State::NoNetwork;
}

State loop_no_condor() {
  check_wm_reset();
  return udp.parsePacket() ? State::Running : State::NoCondor;
}


State loop_running() {
  auto pkt_len = udp.parsePacket();
  udp.read(udp_buffer, UDP_TX_PACKET_MAX_SIZE);
  udp_buffer[pkt_len] = 0;
  char* line = strtok(udp_buffer, "\n\r");
  while (line != 0) {
    char* separator = strchr(line, '=');
    if (separator == 0) continue;
    *separator = 0;
    if (strcmp(line, "airspeed") == 0) {
      separator++;
      auto airspeed_kts = atof(separator) * MS_TO_KTS;
      display.print_3dig_roll(airspeed_kts);
      asi_motor.set_target(kts_to_step(airspeed_kts));
    }
    line = strtok(0, "\n\r");
  }
  return State::Running;
}

State loop_calibration() {
  static char input_line[64];
  static uint8_t input_pos = 0;
  while (Serial.available()) {
    auto new_char = Serial.read();
    input_line[input_pos++] = new_char;
    input_pos %= 64;
    if (new_char == '\n') {
      if(input_line[0] == 'x'){
      input_pos = 0;
      return State::NoNetwork;
      }
      int target = atoi(input_line);
      display.print_msg(String(target), 1);
      asi_motor.set_target(target);
      input_pos = 0;
    }
  }
  return State::Calibration;
}


void loop() {
  asi_motor.loop();

  if (Serial.available() > 0 && state != State::Calibration) {
    display.print_msg("Calibration");
    state = State::Calibration;
  }

  switch (state) {
    case State::NoNetwork:
      state = loop_no_network();
      break;
    case State::NoCondor:
      state = loop_no_condor();
      break;
    case State::Running:
      state = loop_running();
      break;
    case State::Calibration:
      state = loop_calibration();
      break;
  }
}
