#include <Bounce2.h>

#define ARRAYSIZE(x)  (int)(sizeof(x) / sizeof(x[0]))

#define MIDI_CHANNEL 1
#define BOUNCE_MS 10

#define ROTARY_N 4

#define FS_0_PIN 0
#define FS_1_PIN 1
#define FS_2_PIN 2
#define FS_3_PIN 3
#define FS_4_PIN 4

#define POT_0_PIN 14
#define POT_1_PIN 15
#define POT_2_PIN 16

enum MIDIMsgType {
  MIDI_PROGRAM_CHANGE,
  MIDI_CONTROL_CHANGE,
};

#define FS_HOLD_LENGTH 1000
#define FS_HOLD_IGNORE 10000000

struct Message {
  MIDIMsgType type;
  int         message;
};

struct Button {
  int     pin;
  Bounce  bounce;

  Message press;

  boolean msgSent;

  boolean isDown = false;
  long    fallTime = 0;
  long    holdLength = FS_HOLD_IGNORE;
  Message hold;
};

struct Pot {
  int pin;
  int value;
  Message turn[ROTARY_N];
};

Button buttons[] = {
  {
    .pin = FS_0_PIN,
    .bounce = Bounce(),
    .press = {
      .type = MIDI_PROGRAM_CHANGE,
      .message = 0,
    },
    .msgSent = false,
    .isDown = false,
    .fallTime = 0,
    .holdLength = FS_HOLD_IGNORE,
  },
  {
    .pin = FS_1_PIN,
    .bounce = Bounce(),
    .press = {
      .type = MIDI_PROGRAM_CHANGE,
      .message = 1,
    },
    .msgSent = false,
    .isDown = false,
    .fallTime = 0,
    .holdLength = FS_HOLD_IGNORE,
  },
  {
    .pin = FS_2_PIN,
    .bounce = Bounce(),
    .press = {
      .type = MIDI_PROGRAM_CHANGE,
      .message = 2,
    },
    .msgSent = false,
    .isDown = false,
    .fallTime = 0,
    .holdLength = FS_HOLD_IGNORE,
  },
  {
    .pin = FS_3_PIN,
    .bounce = Bounce(),
    .press = {
      .type = MIDI_CONTROL_CHANGE,
      .message = 70,
    },
    .msgSent = false,
    .isDown = false,
    .fallTime = 0,
    .holdLength = FS_HOLD_LENGTH,
    .hold = {
      .type = MIDI_CONTROL_CHANGE,
      .message = 126,
    },
  },
  {
    .pin = FS_4_PIN,
    .bounce = Bounce(),
    .press = {
      .type = MIDI_CONTROL_CHANGE,
      .message = 71,
    },
    .msgSent = false,
    .isDown = false,
    .fallTime = 0,
    .holdLength = FS_HOLD_LENGTH,
    .hold = {
      .type = MIDI_CONTROL_CHANGE,
      .message = 127,
    },
  },
};

Pot pots[] {
  {
    .pin = POT_0_PIN,
    .value = 0,
    .turn = {
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 20,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 23,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 26,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 29,
      },
    },
  },
  {
    .pin = POT_1_PIN,
    .value = 0,
    .turn = {
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 21,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 24,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 27,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 30,
      },
    },
  },
  {
    .pin = POT_2_PIN,
    .value = 0,
    .turn = {
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 22,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 25,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 28,
      },
      {
        .type = MIDI_CONTROL_CHANGE,
        .message = 31,
      },
    },
  },
};

int rotaryPins[] = {32, 31, 30, 29};

void sendMIDIMsg(Message msg, int* value, boolean log) {
  if (log) {
    Serial.print("message:");
    Serial.print(msg.message, DEC);
    if (value) {
      Serial.print("  value:");
      Serial.print(*value, DEC);
    }
  }

  switch (msg.type) {
    case MIDI_PROGRAM_CHANGE:
      usbMIDI.sendProgramChange(msg.message, MIDI_CHANNEL);
      break;
    case MIDI_CONTROL_CHANGE:
      int v = 0;
      if (value) {
        v = *value;
      }
      usbMIDI.sendControlChange(msg.message, v, MIDI_CHANNEL);
      break;
  }
}

int pot_bounce(struct Pot* pot) {
  int newValue = analogRead(pot->pin) / 8;
  if ((newValue - 1 > pot->value) || (newValue + 1 < pot->value)) {
    pot->value = newValue;
  }
  return pot->value;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Cedar initialized...");

  for (int i = 0; i < ARRAYSIZE(buttons); i++) {
    buttons[i].bounce.attach(buttons[i].pin, INPUT_PULLUP);
    buttons[i].bounce.interval(BOUNCE_MS);
  }

  for (int i = 0; i < ARRAYSIZE(rotaryPins); i++) {
    pinMode(rotaryPins[i], INPUT_PULLUP);
  }

  // Read initial pot state
  for (int i = 0; i < ARRAYSIZE(pots); i++) {
    pot_bounce(&pots[i]);
    Serial.print("POT  pin:");
    Serial.print(pots[i].pin, DEC);
    Serial.print(" value:");
    Serial.print(pots[i].value, DEC);
    Serial.println();
  }
}

int rotaryValue = 0;


void loop() {
  // Update all the buttons and pots. There should not be any long
  // delays in loop(), so this runs repetitively at a rate
  // faster than the buttons could be pressed and released.

  long now = millis();

  // Read the rotary switch value.
  int tmp = 0;
  for (tmp = 0; tmp < ARRAYSIZE(rotaryPins); tmp++) {
    if (digitalRead(rotaryPins[tmp]) == LOW) { // Active low
      if (rotaryValue != tmp) {
        rotaryValue = tmp;
        Serial.print("ROT   pin:");
        Serial.print(rotaryPins[tmp]);
        Serial.print(" value:");
        Serial.println(rotaryValue, DEC);
      }
      break;
    }
  }

  // Detect interactions with the buttons and send appropriate
  // messages.
  for (int i = 0; i < ARRAYSIZE(buttons); i++) {
    buttons[i].bounce.update();

    if (buttons[i].isDown &&
        buttons[i].msgSent == false &&
        buttons[i].holdLength != FS_HOLD_IGNORE &&
        now - buttons[i].fallTime > buttons[i].holdLength)
    {
      Serial.print("HOLD  pin:");
      Serial.print(buttons[i].pin, DEC);
      Serial.print(" ");
      sendMIDIMsg(buttons[i].hold, NULL, true);
      buttons[i].msgSent = true;
      Serial.println();
    }

    if (buttons[i].bounce.fell()) {
      Serial.print("FELL  pin:");
      Serial.println(buttons[i].pin, DEC);

      buttons[i].isDown = true;
      buttons[i].fallTime = millis();
      buttons[i].msgSent = false;
    }

    if (buttons[i].bounce.rose()) {
      Serial.print("ROSE  pin:");
      Serial.print(buttons[i].pin, DEC);
      Serial.print(" ");

      if (buttons[i].msgSent == false) {
        sendMIDIMsg(buttons[i].press, NULL, true);
      }
      Serial.println();

      buttons[i].isDown = false;
      buttons[i].msgSent = false;
    }
  }

  // Read and reflect any changes to the pots.
  for (int i = 0; i < ARRAYSIZE(pots); i++) {
    int oldValue = pots[i].value;
    pot_bounce(&pots[i]);
    if (oldValue != pots[i].value) {
      Serial.print("POT   pin:");
      Serial.print(pots[i].pin, DEC);
      Serial.print(" ");
      sendMIDIMsg(pots[i].turn[rotaryValue], &(pots[i].value), true);
      Serial.println();
    }
  }
}
