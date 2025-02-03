#include <FastLED.h>
#include <SmartThing.h>
#include <AsyncUDP.h>

#define PIN GPIO_NUM_14
#define NUM_LEDS 144
#define HALF_LEDS_INDEX 71

enum Mode {
  MODE_RGB,
  MODE_MUSIC,
  MODE_OFF
};

Mode mode;
CRGB leds[NUM_LEDS];
int currentPosition = 0;
int8_t channelsCurrent[] = {0, 0};
byte counter = 0;

AsyncUDP udp;
int8_t channels[2];

void switchMode(Mode mode, bool clear = true);
void onPacket(AsyncUDPPacket &packet);
void addActions();
void setupConfig();
void setPosition(int8_t *position);
void onConfigUpdate();

void rgb();

void setup() {
  addActions();
  setupConfig();

  if (!SmartThing.init("led")) {
    st_log_error("main", "Failed to init SmartThing");
  }

  while(!SmartThing.wifiConnected()) {
    delay(100);
  }

  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  // FastLED.setMaxPowerInVoltsAndMilliamps(5, 5000);
  FastLED.clear(true);

  onConfigUpdate();
  int index = 0;
  for (int i = 1; i < HALF_LEDS_INDEX; i++) {
    for (byte ch = 0; ch < 2; ch++) {
      if (ch == 0) {
        index = 1 - i;
      } else {
        index = i;
      }
      leds[HALF_LEDS_INDEX + index] = CHSV(i, 255, 255);
      FastLED.show();
    }
  }
  FastLED.clear(true);
}

void loop() {
  SmartThing.loop();
  
  switch(mode) {
    case MODE_RGB:
      rgb();
      delay(5);
      break;
    case MODE_OFF:
    default:
      delay(200);
  }
  delay(100);
}

void switchMode(Mode newMode, bool clear) {
  if (newMode == mode) {
    return;
  }

  if (mode == MODE_MUSIC) {
    udp.flush();
    udp.close();
  }

  if (newMode == MODE_MUSIC) {
    if (udp.listen(9090)) {
      udp.onPacket(onPacket);
    }
  }

  mode = newMode;
  if (clear) {
    FastLED.clear(true);
  }
}

void onPacket(AsyncUDPPacket &packet) {
  if (packet.length() == 0 || packet.length() > 10) {
    return;
  }

  if (packet.data()[0] < '0' || packet.data()[0] > '9') {
    switch(packet.data()[0]) {
      case 'c':
        FastLED.clear(true);
        break;
    }  
  } else {
    byte ch = 0;
    int8_t tmp = 0;
    for (int i = 0; i < packet.length(); i++) {
      if (packet.data()[i] == ';') {
        channels[ch] = tmp;
        ch++;
        tmp = 0;
      } else {
        tmp += (packet.data()[i] - '0') + tmp * 10;
      }
    }
    setPosition(channels);
  }
}

void addActions() {
  ActionsManager.add("music", "Music", []() {
    switchMode(MODE_MUSIC);
    return true;
  });
  ActionsManager.add("rgb", "RGB", []() {
    switchMode(MODE_RGB);
    return true;
  });
  ActionsManager.add("turn_off", "Turn off", []() {
    switchMode(MODE_OFF, false);
    for (int i = FastLED.getBrightness(); i >= 0; i--) {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(3);
    }
    return true;
  });
}

void setupConfig() {
  ConfigManager.add("brightness");
  ConfigManager.onConfigUpdate(onConfigUpdate);
}

void onConfigUpdate() {
  FastLED.setBrightness(ConfigManager.getInt("brightness"));
  FastLED.show();
}

void rgb() {
  for (byte ch = 0; ch < 2; ch++) {
    int index = 0;
    for (int i = 0; i < HALF_LEDS_INDEX; i--) {
      if (ch == 0) {
        index = 1 - i;
      } else {
        index = i;
      }
      leds[HALF_LEDS_INDEX + index] = CHSV(counter + 2 * i, 255, 255);
    }
  }
  counter++;
  FastLED.show();
}

void setPosition(int8_t *channels) {
  for (byte ch = 0; ch < 2; ch++) {
    if (channels[ch] > HALF_LEDS_INDEX) {
      channels[ch] = HALF_LEDS_INDEX;
    } else if (channels[ch] < 0) {
      channels[ch] = 0;
    }

    if (channelsCurrent[ch] == channels[ch]) {
      continue;
    }

    if (channels[ch] > channelsCurrent[ch]) {
      int index = 0;
      for (int i = channelsCurrent[ch]; i < channels[ch]; i++) {
        if (ch == 0) {
          index = 1 - i;
        } else {
          index = i;
        }
        leds[HALF_LEDS_INDEX + index] = CHSV(260 + i, 255, 255);
      }
    } else {
      int index = 0;
      for (int i = channelsCurrent[ch]; i >= channels[ch]; i--) {
        if (ch == 0) {
          index = 1 - i;
        } else {
          index = i;
        }
        leds[HALF_LEDS_INDEX + index] = CRGB::Black;
      }
    }

    channelsCurrent[ch] = channels[ch];
  }

  FastLED.show();
}