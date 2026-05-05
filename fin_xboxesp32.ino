#include <Bluepad32.h>

// --- PIN DEFINITIONS---
// LEFT Driver
const int LEFT_IN1  = 26;
const int LEFT_IN2  = 27;
const int LEFT_PWM  = 25;

// RIGHT Driver
const int RIGHT_IN1 = 33;
const int RIGHT_IN2 = 32;
const int RIGHT_PWM = 14;

// STANDBY (Must be HIGH for motors to move)
const int STBY = 13;

// Global controller pointer
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// --- BLUEPAD32 CALLBACKS ---
void onConnectedController(ControllerPtr ctl) {
    Serial.println("\n[!] SUCCESS: Xbox Controller Connected!");
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            break;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    Serial.println("\n[!] DISCONNECT: Controller lost.");
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            break;
        }
    }
}

// --- MOTOR CONTROL FUNCTION ---
void setMotor(int speed, int in1, int in2, int channel) {
    if (speed > 20) { // Forward with small deadzone
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
    } 
    else if (speed < -20) { // Backward
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
    }
    else { // Stop
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
    }
    ledcWrite(channel, abs(speed));
}

void setup() {
    Serial.begin(115200);

    // Initialize Motor Pins
    pinMode(LEFT_IN1, OUTPUT);  pinMode(LEFT_IN2, OUTPUT);
    pinMode(RIGHT_IN1, OUTPUT); pinMode(RIGHT_IN2, OUTPUT);
    pinMode(STBY, OUTPUT);
    
    // Safety: Drivers start enabled
    digitalWrite(STBY, HIGH);

    // PWM Setup
    ledcSetup(0, 5000, 8); // Channel 0: Left
    ledcAttachPin(LEFT_PWM, 0);
    ledcSetup(1, 5000, 8); // Channel 1: Right
    ledcAttachPin(RIGHT_PWM, 1);

    // Bluepad32 Initialization
    BP32.setup(&onConnectedController, &onDisconnectedController);
    
    // Uncomment the line below ONLY if you need to pair a new controller
    // BP32.forgetBluetoothKeys(); 

    Serial.println("System Ready. Waiting for Xbox Controller...");
}

void loop() {
    // Keep the Bluetooth engine running
    BP32.update();

    // Get data from the first controller
    ControllerPtr ctl = myControllers[0];

    if (ctl && ctl->isConnected()) {
        // Get Stick values (-512 to 512)
        int leftStick = ctl->axisY();
        int rightStick = ctl->axisRY();

        // Convert to PWM range (-255 to 255)
        // We flip the map (512, -511) because UP is negative on most controllers
        int leftSpeed  = map(leftStick, 512, -511, -255, 255);
        int rightSpeed = map(rightStick, 512, -511, -255, 255);

        // Send to motors
        setMotor(leftSpeed, LEFT_IN1, LEFT_IN2, 0);
        setMotor(rightSpeed, RIGHT_IN1, RIGHT_IN2, 1);

        // Debugging
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 200) {
            Serial.printf("L-Speed: %d | R-Speed: %d\n", leftSpeed, rightSpeed);
            lastPrint = millis();
        }
    } else {
        // If no controller, stop for safety
        ledcWrite(0, 0);
        ledcWrite(1, 0);
    }
}