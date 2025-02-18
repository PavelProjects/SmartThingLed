#include <FastLED.h>
#include <SmartThing.h>
#include <AsyncUDP.h>

#define PIN GPIO_NUM_14
#define NUM_LEDS 144
#define HALF_LEDS_INDEX 71

enum Mode {
  MODE_MUSIC,
  MODE_RGB,
  MODE_OFF
};

Mode mode;
CRGB leds[NUM_LEDS];

AsyncUDP udp;
int8_t channels[2] = {0, 0};
int8_t channelsCurrent[] = {0, 0};
int musicBase = 0;
byte counter = 0;


void switchMode(Mode newMode);
void onPacket(AsyncUDPPacket &packet);
void addActions();
void music(int8_t *position);
void onConfigUpdate();

void rgb();

void setup() {
  addActions();
  SensorsManager.add("mode", []() {
    switch(mode) {
      case MODE_RGB:
        return "rgb";
      case MODE_MUSIC:
        return "music";
      default:
        return "off";
    }
  });
  ConfigManager.add("brightness");
  ConfigManager.add("base_color");
  ConfigManager.onConfigUpdate(onConfigUpdate);

  FastLED.addLeds<WS2812, PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 5000);
  FastLED.clear(true);

  if (!SmartThing.init("led")) {
    st_log_error("main", "Failed to init SmartThing");
  }
  onConfigUpdate();

  while(!SmartThing.wifiConnected()) {
    delay(100);
  }

  int index = 0;
  for (int i = 1; i < HALF_LEDS_INDEX; i++) {
    for (byte ch = 0; ch < 2; ch++) {
      if (ch == 0) {
        index = 1 - i;
      } else {
        index = i;
      }
      leds[HALF_LEDS_INDEX + index] = CHSV(i, 255, 255);
    }
    FastLED.show();
  }
  delay(100);
  FastLED.clear(true);

  if (udp.listenMulticast(IPAddress(224, 1, 1, 1), 7780)) {
    udp.onPacket(onPacket);
  }
}

void loop() {
    if (mode == MODE_RGB) {
    rgb();
    delay(10);
  } else {
    delay(200);
  }
}

void switchMode(Mode newMode) {
  if (mode == MODE_OFF) {
    FastLED.setBrightness(ConfigManager.getInt("brightness"));
  }
  
  mode = newMode;
  delay(50);

  if (mode == MODE_OFF) {
    for (int i = FastLED.getBrightness(); i >= 0; i--) {
      FastLED.setBrightness(i);
      FastLED.show();
      delay(3);
    }
  } else if (mode == MODE_MUSIC) {
    for (byte ch = 0; ch < 2; ch++) {
      channels[ch] = 0;
      channelsCurrent[ch] = 0;
    }
  }

  FastLED.clear(true);
}

void onPacket(AsyncUDPPacket &packet) {
  if (mode != MODE_MUSIC || packet.length() == 0 || packet.length() > 10) {
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
    music(channels);
  }
  udp.flush();
}

void addActions() {
  ActionsManager.add("rgb", "RGB", []() {
    switchMode(MODE_RGB);
    return true;
  });
  ActionsManager.add("music", "Music", []() {
    switchMode(MODE_MUSIC);
    return true;
  });
  ActionsManager.add("off", "Turn off", []() {
    switchMode(MODE_OFF);
    return true;
  });
}

void onConfigUpdate() {
  FastLED.setBrightness(ConfigManager.getInt("brightness", 100));
  FastLED.show();

  int newBase = ConfigManager.getInt("base_color");
  if (newBase != musicBase) {
    musicBase = newBase;
    for (byte ch = 0; ch < 2; ch++) {
      channels[ch] = 0;
      channelsCurrent[ch] = 0;
    }
  }
}

void rgb() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(counter + 2 * i, 255, 255);
  }
  counter++;
  FastLED.show();
}

void music(int8_t *channels) {
  for (byte ch = 0; ch < 2; ch++) {
    if (channels[ch] > HALF_LEDS_INDEX) {
      channels[ch] = HALF_LEDS_INDEX;
    } else if (channels[ch] < 0) {
      channels[ch] = 0;
    }

    if (channelsCurrent[ch] == channels[ch]) {
      continue;
    }
    
    int index = 0;
    if (channels[ch] > channelsCurrent[ch]) {
      for (int i = channelsCurrent[ch]; i < channels[ch]; i++) {
        if (ch == 0) {
          index = 1 - i;
        } else {
          index = i;
        }
        leds[HALF_LEDS_INDEX + index] = CHSV(musicBase + i, 255, 255);
      }
    } else {
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