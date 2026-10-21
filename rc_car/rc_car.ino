// RC car controller (L298N, two DC motors, differential drive)
// Receives single-character commands over Serial from the Processing GUI.
//
// Commands:
//   'F' forward   'B' backward   'L' turn left   'R' turn right   'S' stop
//   'Q' spin left 'E' spin right (wheels in opposite directions)
//   'Z' curve left 'X' curve right (both forward, different speeds)
//   '0'-'9'       set speed (0 = slowest usable, 9 = full)

// Motor A (left side)
int enA = 10;  // PWM -> L298N ENA (white)
int in1 = 2;   //     -> L298N IN1 (orange)
int in2 = 3;   //     -> L298N IN2 (yellow)

// Motor B (right side)
int enB = 9;   // PWM -> L298N ENB (red)
int in3 = 4;   //     -> L298N IN3 (brown)
int in4 = 5;   //     -> L298N IN4 (black)

int speed = 200;        // current PWM speed (0-255)
const int MIN_PWM = 80; // below this most motors just hum

// Motors are mounted mirror-image, so one side is wired "backwards".
// Set to -1 to flip a side if it spins the wrong way on 'F'.
const int LEFT_FLIP  = -1;
const int RIGHT_FLIP = 1;

// Inner wheel speed (% of outer) for Z/X curves. Lower = tighter curve.
const int CURVE_PCT = 50;

void setup() {
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(enB, OUTPUT);
    pinMode(in3, OUTPUT);
    pinMode(in4, OUTPUT);

    Serial.begin(9600);
    stopCar();
}

void loop() {
    if (Serial.available() > 0) {
        char cmd = Serial.read();

        if (cmd >= '0' && cmd <= '9') {
            speed = map(cmd - '0', 0, 9, MIN_PWM, 255);
            Serial.print("Speed: ");
            Serial.println(speed);
            return;
        }

        switch (cmd) {
            case 'F': drive( 1,  1); break;
            case 'B': drive(-1, -1); break;
            case 'L': drive( 1,  0); break;  // pivot left (one wheel)
            case 'R': drive( 0,  1); break;  // pivot right (one wheel)
            case 'Q': drive( 1, -1); break;  // spin left in place (both wheels)
            case 'E': drive(-1,  1); break;  // spin right in place (both wheels)
            case 'Z': driveCurve(1, 1, 100, CURVE_PCT); break;  // curve left
            case 'X': driveCurve(1, 1, CURVE_PCT, 100); break;  // curve right
            case 'S': stopCar();     break;
            default: return;                 // ignore newlines, etc.
        }
        Serial.print("Cmd: ");
        Serial.println(cmd);
    }
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
