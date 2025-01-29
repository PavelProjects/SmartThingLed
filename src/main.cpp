#include <FastLED.h>
#include <SmartThing.h>
#include <AsyncUDP.h>

#define PIN GPIO_NUM_14
#define NUM_LEDS 144
#define HALF_LEDS_INDEX 72
#define STEP 2

#define PORT 8080

CRGB leds[NUM_LEDS];
int currentPosition = 0;
CRGB color = CRGB::Black;

int sockfd = 0;
AsyncUDP udp;

void addActions();
void setupConfig();
void handleCommand(char command);
void fillColor(uint8_t from, uint8_t to);
void setPosition(int position);
void onConfigUpdate();
uint8_t getBrightnessFromConfig();
void loadColorFromConfig();

void setup() {
  addActions();
  setupConfig();

  if (!SmartThing.init("led")) {
    st_log_error("main", "Failed to init SmartThing");
  }

  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  onConfigUpdate();
  FastLED.clear(true);

  while (!SmartThing.wifiConnected()) {
    delay(100);
  }

  if (udp.listen(9090)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      if (packet.length() < 1) {
        return;
      }
      if (packet.data()[0] < '0' || packet.data()[0] > '9') {
        handleCommand(packet.data()[0]);      
      } else {
        setPosition(atoi((char*)packet.data()));
      }
    });
  }
}

void loop() {
  delay(100);
}

void setPosition(int position) {
  if (position > HALF_LEDS_INDEX) {
    position = HALF_LEDS_INDEX;
  } else if (position < 0) {
    position = 0;
  }

  if (currentPosition == position) {
    return;
  }

  if (position > currentPosition) {
    fillColor(currentPosition, position);
  } else {
    for (int i = currentPosition; i >= position; i--) {
      leds[HALF_LEDS_INDEX + i] = CRGB::Black;
      leds[HALF_LEDS_INDEX - 1 - i] = leds[HALF_LEDS_INDEX + i];
    }
  }

  FastLED.show();
  currentPosition = position;
}

/*
  c - clear
  f - fill
  e - enable
  d - disable
*/
void handleCommand(char command) {
  switch(command) {
    case 'c':
      FastLED.clear(true);
      break;
    case 'f':
      fillColor(0, HALF_LEDS_INDEX);
      FastLED.show();
      break;
    case 'e':
      ActionsManager.call("turn_on");
      break;
    case 'd':
      ActionsManager.call("turn_off");
      break;
  }
}

void fillColor(uint8_t from, uint8_t to) {
  for (int i = from; i < to; i++) {
    leds[HALF_LEDS_INDEX + i] = color == CRGB::Black ? CHSV(245 - i, 255, 255) : color;
    leds[HALF_LEDS_INDEX - 1 - i] = leds[HALF_LEDS_INDEX + i];
  }
}

void addActions() {
  ActionsManager.add("turn_off", "Turn off", []() {
    for (int i = FastLED.getBrightness(); i >= 0; i--) {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(3);
    }
    return true;
  });
  ActionsManager.add("turn_on", "Turn on", []() {
    uint8_t conf = getBrightnessFromConfig();
    if (conf == FastLED.getBrightness()) {
      return true;
    }

    for (int i = 0; i < conf; i++) {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(3);
    }

    return true;
  });
}

void setupConfig() {
  ConfigManager.addConfigEntry("brightness");
  ConfigManager.addConfigEntry("color");

  ConfigManager.onConfigUpdate(onConfigUpdate);
}

void onConfigUpdate() {
  FastLED.setBrightness(getBrightnessFromConfig());
  loadColorFromConfig();
  fillColor(0, currentPosition);
}

uint8_t getBrightnessFromConfig() {
  String brt = ConfigManager.getConfigValue("brightness");
  uint8_t value = 0;
  if (brt.length() > 0) {
    return brt.toInt();
  }
  return 0;
}

void loadColorFromConfig() {
  String conf = ConfigManager.getConfigValue("color");
  if (conf.isEmpty() || conf.length() > 11 || conf.equals("g")) {
    color = CRGB::Black;
  } else {
    int16_t r = -1, g = -1, b = -1;
    String buff;
    for (uint8_t i = 0; i < conf.length(); i++) {
      if (conf.charAt(i) == ',') {
        if (r == -1) {
          r = buff.toInt();
        } else if (g == -1) {
          g = buff.toInt();
        }
        buff.clear();
      } else {
        buff += conf.charAt(i);
      }
    }
    b = buff.toInt();

    if (r == -1 || g == -1 || b == -1) {
      st_log_error("main", "Malformed color: %s (parsed to %d %d %d)", conf.c_str(), r, g, b);
      color = CRGB::Black;
    } else {
      color = CRGB(r, g ,b);
    }
  }
}