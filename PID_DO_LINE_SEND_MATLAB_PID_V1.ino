#include <WiFi.h>
#include <PID_v1.h>   // <-- thêm thư viện PID_v1

// -------------------- WiFi Config --------------------
const char* ssid = "Galaxy";
const char* password = "03102005";

IPAddress local_IP(10, 135, 155, 150);
IPAddress gateway(10, 135, 155, 120);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);
WiFiClient client;

bool connectedToMatlab = false;
int speed = 50; //50-90

// -------------------- Motor & Sensor Config --------------------
#define ENA 32
#define IN1 33
#define IN2 25
#define ENB 26
#define IN3 27
#define IN4 14

int R_speed = speed;
int L_speed = speed;
int GTBD = speed;

int s1 = 19;
int s2 = 18;
int s3 = 5;
int s4 = 17;
int s5 = 16;

int l1, l2, l3, l4, l5;

// -------------------- PID Variables --------------------
int error;
double Setpoint = 0;       // line giữa = 0
double Input, Output;      // Input = error, Output = PID_value

double Kp = 0.0; 
double Ki = 0.0; 
double Kd = 0.0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, REVERSE); //DIRECT

// -------------------- MOTOR CONTROL --------------------
void motorLeft(int speed) {
  if (speed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWriteChannel(0, speed);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWriteChannel(0, -speed);
  }
}

void motorRight(int speed) {
  if (speed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWriteChannel(1, speed*0.8);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWriteChannel(1, -speed*0.8);
  }
}

// -------------------- SENSOR PROCESSING --------------------
void errorCalculation() {
  l5 = digitalRead(s1);
  l4 = digitalRead(s2);
  l3 = digitalRead(s3);
  l2 = digitalRead(s4);
  l1 = digitalRead(s5);

  if (l5 == 1 && l4 == 1 && l3 == 1 && l2 == 1 && l1 == 0) error = 4;
  else if (l5 == 1 && l4 == 1 && l3 == 1 && l2 == 0 && l1 == 0) error = 3;
  else if (l5 == 1 && l4 == 1 && l3 == 1 && l2 == 0 && l1 == 1) error = 2;
  else if (l5 == 1 && l4 == 1 && l3 == 0 && l2 == 0 && l1 == 1) error = 1;
  else if (l5 == 1 && l4 == 1 && l3 == 0 && l2 == 1 && l1 == 1) error = 0;
  else if (l5 == 1 && l4 == 0 && l3 == 0 && l2 == 1 && l1 == 1) error = -1;
  else if (l5 == 1 && l4 == 0 && l3 == 1 && l2 == 1 && l1 == 1) error = -2;
  else if (l5 == 0 && l4 == 0 && l3 == 1 && l2 == 1 && l1 == 1) error = -3;
  else if (l5 == 0 && l4 == 1 && l3 == 1 && l2 == 1 && l1 == 1) error = -4;
  else if (l5 == 0 && l4 == 0 && l3 == 0 && l2 == 0 && l1 == 0) error = -5;
}

// -------------------- SPEED CONTROL --------------------
void speedControl() {
  L_speed = GTBD - Output;
  R_speed = GTBD + Output;

  L_speed = constrain(L_speed, -90, 90);
  R_speed = constrain(R_speed, -90, 90);

  motorLeft(L_speed);
  motorRight(R_speed);
}

// -------------------- MATLAB COMMUNICATION --------------------
void sendMatlab() {
  if (!connectedToMatlab) {
    WiFiClient newClient = server.available();
    if (newClient) {
      client = newClient;
      connectedToMatlab = true;
      Serial.println("✅ MATLAB đã kết nối!");
    }
  }

  if (connectedToMatlab && client.available()) {
    String msg = client.readStringUntil('\n');
    msg.trim();

    if (msg.startsWith("PID:")) {
      msg.remove(0, 4);
      int c1 = msg.indexOf(',');
      int c2 = msg.indexOf(',', c1 + 1);
      if (c1 > 0 && c2 > c1) {
        Kp = msg.substring(0, c1).toFloat();
        Ki = msg.substring(c1 + 1, c2).toFloat();
        Kd = msg.substring(c2 + 1).toFloat();
        myPID.SetTunings(Kp, Ki, Kd); // cập nhật PID_v1
        Serial.printf("📥 Nhận PID mới: %.2f %.2f %.2f\n", Kp, Ki, Kd);
      } else {
        Serial.println("⚠️ Sai định dạng PID từ MATLAB!");
      }
      client.printf("ESP32 Send: Kp=%.2f Ki=%.2f Kd=%.2f\n", Kp, Ki, Kd);
    }
  }

  static unsigned long lastSend = 0;
  if (connectedToMatlab && millis() - lastSend > 100) {
    lastSend = millis();
    client.printf("DATA:%.2f,%.2f,%.2f,%.2f\n", (float)error, Output, (float)L_speed, (float)R_speed);
  }

  if (connectedToMatlab && !client.connected()) {
    Serial.println("❌ MATLAB ngắt kết nối.");
    client.stop();
    connectedToMatlab = false;
    server.begin();
  }
}

// -------------------- SETUP --------------------
void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(s1, INPUT);
  pinMode(s2, INPUT);
  pinMode(s3, INPUT);
  pinMode(s4, INPUT);
  pinMode(s5, INPUT);

  ledcAttachChannel(ENA, 50, 8, 0);
  ledcAttachChannel(ENB, 50, 8, 1);

  Serial.begin(115200);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi kết nối thành công!");
    Serial.print("📡 IP: ");
    Serial.println(WiFi.localIP());
    server.begin();
    Serial.println("🌐 Server started!");
  } else {
    Serial.println("\n⚠️ Không thể kết nối WiFi.");
  }

  // -------------------- PID_v1 setup --------------------
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(-120, 120);   // giống constrain bạn dùng
  myPID.SetSampleTime(10);            // 10ms

  Serial.println("✅ PID Line Follower sẵn sàng (PID_v1).");
}

// -------------------- LOOP --------------------
unsigned long lastPIDTime = 0;

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastPIDTime >= 10) {
    lastPIDTime = currentTime;
    errorCalculation();

    // PID_v1 input/output
    Input = error;
    myPID.Compute();  // Output sẽ được cập nhật tự động

    speedControl();
  }

  sendMatlab();
}
