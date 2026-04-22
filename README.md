# ESP32 LINE FOLLOWING ROBOT WITH PID & MATLAB MONITORING
Hệ thống robot dò line sử dụng **ESP32 + PID Controller**, kết hợp với **MATLAB** để giám sát và phân tích dữ liệu theo thời gian thực thông qua giao thức **TCP/IP (WiFi)**.
## Tổng quan hệ thống
Hệ thống gồm 2 phần chính:
### 🔹 1. ESP32 (Server - Robot Controller)
- Đọc dữ liệu từ cảm biến line (5 sensor)
- Tính toán sai số (error)
- Áp dụng PID control
- Điều khiển tốc độ động cơ trái/phải
- Gửi dữ liệu realtime sang MATLAB qua TCP

### 🔹 2. MATLAB (Client - Monitoring Tool)
- Kết nối TCP tới ESP32
- Nhận dữ liệu realtime
- Hiển thị đồ thị:
  - Error
  - PID Output
  - Motor Left PWM
  - Motor Right PWM

## ⚙️ Phần cứng sử dụng
- ESP32
- Driver động cơ (L298N hoặc tương đương)
- 2 động cơ DC
- 5 cảm biến dò line (Digital)
- Nguồn cấp phù hợp

## 🔌 Sơ đồ chân ESP32

### Motor Driver
| Chức năng | GPIO |
|----------|------|
| ENA      | 32   |
| IN1      | 33   |
| IN2      | 25   |
| ENB      | 26   |
| IN3      | 27   |
| IN4      | 14   |

### Line Sensors
| Sensor | GPIO |
|--------|------|
| S1     | 19   |
| S2     | 18   |
| S3     | 5    |
| S4     | 17   |
| S5     | 16   |

##  Thuật toán điều khiển
### 🔹 Tính sai số (Error)
Giá trị error được xác định dựa trên trạng thái 5 cảm biến:
- Lệch trái → error âm  
- Lệch phải → error dương  
- Ở giữa → error = 0  

### 🔹 PID Controller
u(t) = Kp * e(t) + Ki * ∫e(t)dt + Kd * de(t)/dt
Trong đó:
- e(t): sai số (error)
- u(t): tín hiệu điều khiển (PID output)

### 🔹 Điều khiển động cơ
- Motor Left  = BaseSpeed + PID_Output  
- Motor Right = BaseSpeed - PID_Output  

## Kết nối WiFi
ESP32 hoạt động như **TCP Server**:
```cpp
WiFiServer server(80);
```
## Link 
[🎥 Video Demo Sản Phẩm](https://youtube.com/shorts/ThaLz9AZ7bE?feature=share)
[📄Xem bài báo](https://ejournal.ptti.web.id/index.php/jfsc/article/view/368)
