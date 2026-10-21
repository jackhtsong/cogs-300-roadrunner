// Simple GUI for the follow-me robot (rc_car.ino).
// One button turns follow mode on/off; everything the Arduino prints
// is shown in the log below, like the Serial Monitor.
// Close the Arduino Serial Monitor before running this — only one program can use the port.

import processing.serial.*;

Serial port;
String status = "Not connected";
boolean followOn = false;  // mirrored from the Arduino's "Mode:" messages

// Set this to your Arduino's index from the printed list, or -1 to auto-pick the last port
int PORT_INDEX = -1;

ArrayList<String> log = new ArrayList<String>();
final int MAX_LINES = 200;

int btnX = 30, btnY = 20, btnW = 160, btnH = 50;
int logY = 90;

void setup() {
  size(600, 500);
  textFont(createFont("Monospaced", 13));

  printArray(Serial.list());
  String[] ports = Serial.list();
  if (ports.length > 0) {
    int idx = PORT_INDEX >= 0 ? PORT_INDEX : ports.length - 1;
    try {
      port = new Serial(this, ports[idx], 9600);
      port.bufferUntil('\n');
      status = "Connected: " + ports[idx];
    } catch (Exception e) {
      status = "Failed to open " + ports[idx] + " (busy? close Serial Monitor)";
      println("Serial error: " + e.getMessage());
    }
  }
}

void draw() {
  background(30);

  // on/off button
  noStroke();
  fill(followOn ? color(60, 180, 90) : color(180, 60, 60));
  rect(btnX, btnY, btnW, btnH, 10);
  fill(255);
  textAlign(CENTER, CENTER);
  textSize(18);
  text(followOn ? "ON" : "OFF", btnX + btnW / 2, btnY + btnH / 2);

  textAlign(LEFT, CENTER);
  textSize(13);
  fill(160);
  text(status, btnX + btnW + 20, btnY + btnH / 2);

  // log panel (newest at the bottom)
  fill(15);
  rect(10, logY, width - 20, height - logY - 10);
  fill(200, 255, 200);
  textAlign(LEFT, TOP);
  int lineH = 16;
  int visible = (height - logY - 20) / lineH;
  synchronized (log) {
    int start = max(0, log.size() - visible);
    for (int i = start; i < log.size(); i++) {
      text(log.get(i), 18, logY + 5 + (i - start) * lineH);
    }
  }
}

void mousePressed() {
  if (mouseX > btnX && mouseX < btnX + btnW && mouseY > btnY && mouseY < btnY + btnH) {
    if (port == null) return;
    // 'T' turns follow on (only sent when off), 'M' forces off + stop
    port.write(followOn ? 'M' : 'T');
  }
}

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  line = trim(line);
  if (line.length() == 0) return;

  if (line.equals("Mode: FOLLOW")) followOn = true;
  if (line.equals("Mode: MANUAL")) followOn = false;

  synchronized (log) {
    log.add(line);
    if (log.size() > MAX_LINES) log.remove(0);
  }
}
