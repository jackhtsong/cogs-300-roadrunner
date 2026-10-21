// RC car controller (L298N, two DC motors, differential drive)
// Receives single-character commands over Serial from the Processing GUI.
//
// Commands (type in the Serial Monitor):
//   'F' forward   'B' backward   'S' stop
//   (turning commands L/R/Q/E/Z/X are commented out for now)
//   '0'-'9'       set speed (0 = slowest usable, 9 = full)
//   'T'           toggle autonomous follow mode on/off
//   'M'           force manual mode + stop
// In follow mode 'S' is ignored (the GUI sends it on every key release);
// F or B takes back manual control.

// Motor A (left side)
int enA = 10;  // PWM -> L298N ENA (white)
int in1 = 2;   //     -> L298N IN1 (orange)
int in2 = 3;   //     -> L298N IN2 (yellow)

// Motor B (right side)
int enB = 9;   // PWM -> L298N ENB (red)
int in3 = 4;   //     -> L298N IN3 (brown)
int in4 = 5;   //     -> L298N IN4 (black)

// Ultrasonic sensor
int trigPin = A5;
int echoPin = A4;

int speed = 200;        // current PWM speed (0-255)
const int MIN_PWM = 80; // below this most motors just hum

// Motors are mounted mirror-image, so one side is wired "backwards".
// Set to -1 to flip a side if it spins the wrong way on 'F'.
const int LEFT_FLIP  = -1;
const int RIGHT_FLIP = 1;

// Inner wheel speed (% of outer) for Z/X curves. Lower = tighter curve.
const int CURVE_PCT = 50;

// Follow-me P-controller
// Starts in follow mode so it works with just the Serial Monitor.
// Type T to toggle, M for manual + stop.
bool followMode = true;
const float SET_POINT = 10.0;   // target distance (cm)
const float KP        = 12.0;   // PWM per cm of error — tune this
const float DEADBAND  = 1.5;    // within +/- this many cm, just stop
const float MAX_RANGE = 150.0;  // beyond this treat target as lost
unsigned long lastPing = 0;

void setup() {
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(enB, OUTPUT);
    pinMode(in3, OUTPUT);
    pinMode(in4, OUTPUT);

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    Serial.begin(9600);
    stopCar();
    Serial.println(followMode ? "Mode: FOLLOW" : "Mode: MANUAL");
}

void loop() {
    if (Serial.available() > 0) {
        handleCommand(Serial.read());
    }
    if (millis() - lastPing >= 60) {  // ~16 Hz, lets echoes die out
        lastPing = millis();
        float d = readDistanceCm();
        printDistance(d);
        if (followMode) followStep(d);
        else Serial.println();
    }
}

void handleCommand(char cmd) {
    if (cmd == 'T' || cmd == 'M') {
        followMode = (cmd == 'T') ? !followMode : false;
        stopCar();
        Serial.println(followMode ? "Mode: FOLLOW" : "Mode: MANUAL");
        return;
    }
    if (cmd == 'S' && followMode) return;
    if (followMode && strchr("FB", cmd) && cmd != 0) {
        followMode = false;
        Serial.println("Mode: MANUAL");
    }

    if (cmd >= '0' && cmd <= '9') {
        speed = map(cmd - '0', 0, 9, MIN_PWM, 255);
        Serial.print("Speed: ");
        Serial.println(speed);
        return;
    }

    switch (cmd) {
        case 'F': drive( 1,  1); break;
        case 'B': drive(-1, -1); break;
        // Turning disabled for now — only forward/backward.
        // case 'L': drive( 1,  0); break;  // pivot left (one wheel)
        // case 'R': drive( 0,  1); break;  // pivot right (one wheel)
        // case 'Q': drive( 1, -1); break;  // spin left in place (both wheels)
        // case 'E': drive(-1,  1); break;  // spin right in place (both wheels)
        // case 'Z': driveCurve(1, 1, 100, CURVE_PCT); break;  // curve left
        // case 'X': driveCurve(1, 1, CURVE_PCT, 100); break;  // curve right
        case 'S': stopCar();     break;
        default: return;                 // ignore newlines, etc.
    }
    Serial.print("Cmd: ");
    Serial.println(cmd);
}

// Returns distance in cm, or -1 if no echo.
float readDistanceCm() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    unsigned long us = pulseIn(echoPin, HIGH, 30000);  // ~5 m timeout
    if (us == 0) return -1;
    return us * 0.0343 / 2.0;
}

// "D: <cm>" (parsed by the GUI), or "D: -1" when there's no echo
void printDistance(float d) {
    Serial.print("D: ");
    Serial.print(d, 1);
    Serial.print(d < 0 ? "  (no echo)" : " cm");
}

void followStep(float d) {
    if (d < 0 || d > MAX_RANGE) {  // nothing in front: don't chase
        stopCar();
        Serial.println("  | lost target, stopped");
        return;
    }

    float error = d - SET_POINT;      // + = too far, drive forward
    if (abs(error) < DEADBAND) {
        stopCar();
        Serial.println("  | at set point, stopped");
        return;
    }

    float output = KP * error;
    int dir = output > 0 ? 1 : -1;
    // map |output| into the range the motors actually move at
    int pwm = constrain((int)abs(output), 0, 255);
    pwm = map(pwm, 0, 255, MIN_PWM, 255);
    setMotor(enA, in1, in2, LEFT_FLIP * dir, pwm);
    setMotor(enB, in3, in4, RIGHT_FLIP * dir, pwm);

    Serial.print("  | err ");
    Serial.print(error, 1);
    Serial.print(dir > 0 ? "  FWD pwm " : "  BACK pwm ");
    Serial.println(pwm);
}

// dir: 1 = forward, -1 = backward, 0 = stop
void setMotor(int en, int pinA, int pinB, int dir, int pwm) {
    if (dir > 0) {
        digitalWrite(pinA, LOW);
        digitalWrite(pinB, HIGH);
    } else if (dir < 0) {
        digitalWrite(pinA, HIGH);
        digitalWrite(pinB, LOW);
    } else {
        digitalWrite(pinA, LOW);
        digitalWrite(pinB, LOW);
    }
    analogWrite(en, dir == 0 ? 0 : pwm);
}

void drive(int leftDir, int rightDir) {
    driveCurve(leftDir, rightDir, 100, 100);
}

// Like drive(), but each side runs at a percentage of the current speed.
// The slower side is kept at or above MIN_PWM so it doesn't stall.
void driveCurve(int leftDir, int rightDir, int leftPct, int rightPct) {
    int leftPwm  = max(speed * leftPct  / 100, MIN_PWM);
    int rightPwm = max(speed * rightPct / 100, MIN_PWM);
    setMotor(enA, in1, in2, LEFT_FLIP * leftDir, leftPwm);
    setMotor(enB, in3, in4, RIGHT_FLIP * rightDir, rightPwm);
}

void stopCar() {
    drive(0, 0);
}
