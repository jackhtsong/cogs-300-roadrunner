// Processing GUI for the RC car.
// Click the buttons or use arrow keys / WASD (hold to drive, release to stop).
// Number keys 0-9 or the slider set speed.
// F toggles autonomous follow mode (keeps ~25 cm from an object in front).

import processing.serial.*;

Serial port;
String status = "Not connected";
char currentCmd = 'S';
int speedLevel = 7;
boolean followMode = false;  // mirrored from the Arduino's "Mode:" messages
float distance = -1;         // latest ultrasonic reading (cm)
boolean toggleHeld = false;  // ignore key auto-repeat on F

// Set this to your Arduino's index from the printed list, or -1 to auto-pick the last port
int PORT_INDEX = -1;

int btnSize = 90;
int cx, cy;

void setup() {
  size(500, 500);
  textAlign(CENTER, CENTER);
  cx = width / 2;
  cy = height / 2 - 30;

  printArray(Serial.list());
  String[] ports = Serial.list();
  if (ports.length > 0) {
    int idx = PORT_INDEX >= 0 ? PORT_INDEX : ports.length - 1;
    try {
      port = new Serial(this, ports[idx], 9600);
      port.bufferUntil('\n');
      status = "Connected: " + ports[idx];
      delay(2000); // Arduino resets when serial opens
      sendSpeed();
    } catch (Exception e) {
      status = "Failed to open " + ports[idx];
    }
  }
}

void draw() {
  background(30);

  drawButton("▲", cx, cy - btnSize - 10, 'F');
  drawButton("▼", cx, cy + btnSize + 10, 'B');
  drawButton("◀", cx - btnSize - 10, cy, 'L');
  drawButton("▶", cx + btnSize + 10, cy, 'R');
  drawButton("■", cx, cy, 'S');

  // speed slider
  int sx = 100, sy = height - 90, sw = width - 200;
  stroke(120);
  strokeWeight(4);
  line(sx, sy, sx + sw, sy);
  noStroke();
  fill(80, 180, 255);
  ellipse(sx + sw * speedLevel / 9.0, sy, 24, 24);
  fill(220);
  textSize(16);
  text("Speed: " + speedLevel, cx, sy - 25);

  textSize(13);
  fill(160);
  text(status, cx, height - 40);
  text("Arrows/WASD drive · Q/E spin · Z/X curve · 0-9 speed · F follow · space stop", cx, height - 20);

  // mode + distance
  textSize(18);
  fill(followMode ? color(80, 220, 120) : color(220));
  text(followMode ? "FOLLOW MODE" : "MANUAL", cx, 25);
  textSize(14);
  fill(180);
  text(distance < 0 ? "Distance: --" : "Distance: " + nf(distance, 0, 1) + " cm", cx, 48);

  // stop when mouse button released
  if (!mousePressed && !keyPressed && currentCmd != 'S') {
    send('S');
  }
}

void drawButton(String label, int x, int y, char cmd) {
  boolean active = currentCmd == cmd;
  fill(active ? color(80, 180, 255) : color(60));
  rectMode(CENTER);
  rect(x, y, btnSize, btnSize, 12);
  fill(255);
  textSize(36);
  text(label, x, y);
}

char buttonAt(int x, int y) {
  if (inBox(x, y, cx, cy - btnSize - 10)) return 'F';
  if (inBox(x, y, cx, cy + btnSize + 10)) return 'B';
  if (inBox(x, y, cx - btnSize - 10, cy)) return 'L';
  if (inBox(x, y, cx + btnSize + 10, cy)) return 'R';
  if (inBox(x, y, cx, cy)) return 'S';
  return 0;
}

boolean inBox(int px, int py, int bx, int by) {
  return abs(px - bx) < btnSize / 2 && abs(py - by) < btnSize / 2;
}

void mousePressed() {
  char c = buttonAt(mouseX, mouseY);
  if (c != 0) send(c);
  updateSlider();
}

void mouseDragged() {
  updateSlider();
}

void updateSlider() {
  int sx = 100, sy = height - 90, sw = width - 200;
  if (abs(mouseY - sy) < 20 && mouseX >= sx - 10 && mouseX <= sx + sw + 10) {
    int lvl = constrain(round((mouseX - sx) * 9.0 / sw), 0, 9);
    if (lvl != speedLevel) {
      speedLevel = lvl;
      sendSpeed();
    }
  }
}

void keyPressed() {
  if (key >= '0' && key <= '9') {
    speedLevel = key - '0';
    sendSpeed();
    return;
  }
  if (key == 'f' || key == 'F') {
    if (!toggleHeld && port != null) port.write('T');
    toggleHeld = true;
    return;
  }
  if (key == ' ' && followMode) {
    if (port != null) port.write('M');
    return;
  }
  char c = 0;
  if (keyCode == UP    || key == 'w' || key == 'W') c = 'F';
  if (keyCode == DOWN  || key == 's' || key == 'S') c = 'B';
  if (keyCode == LEFT  || key == 'a' || key == 'A') c = 'L';
  if (keyCode == RIGHT || key == 'd' || key == 'D') c = 'R';
  if (key == 'q' || key == 'Q') c = 'Q';
  if (key == 'e' || key == 'E') c = 'E';
  if (key == 'z' || key == 'Z') c = 'Z';
  if (key == 'x' || key == 'X') c = 'X';
  if (key == ' ') c = 'S';
  if (c != 0 && c != currentCmd) send(c);  // avoid flooding on key repeat
}

void keyReleased() {
  if (key == 'f' || key == 'F') toggleHeld = false;
  send('S');  // ignored by the Arduino while in follow mode
}

void send(char c) {
  currentCmd = c;
  if (port != null) port.write(c);
}

void sendSpeed() {
  if (port != null) port.write(char('0' + speedLevel));
}

void serialEvent(Serial p) {
  // optional: read Arduino debug output
  String line = p.readStringUntil('\n');
  if (line == null) return;
  line = trim(line);
  if (line.startsWith("D: ")) {
    distance = float(split(line, ' ')[1]);  // "D: 23.4 cm" -> 23.4
    return;  // too frequent to print
  }
  if (line.equals("Mode: FOLLOW")) followMode = true;
  if (line.equals("Mode: MANUAL")) followMode = false;
  println("Arduino: " + line);
}
