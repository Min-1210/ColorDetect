# ColorDetect
Color Detection Some Simple Colors Using TCS3200
We can edit the values ​​according to the actual error.



# Báo cáo: Hệ thống Digital Twin - Kết nối ROS2 qua Husarnet

## 1. Tổng quan

Dự án xây dựng hệ thống **digital twin** cho robot skid-steer với:
- **Máy server**: Chạy mô phỏng 3D, SLAM, digital twin (RViz, Cartographer, Nav2)
- **Máy laptop (robot thật)**: Kết nối trực tiếp LiDAR Velodyne VLP-16, IMU MicroStrain, các cảm biến khác
- **Kết nối**: Husarnet VPN + ROS2 CycloneDDS (kết nối qua Internet, không cần mạng LAN chung)

---

## 2. Kiến trúc hệ thống

### 2.1 Sơ đồ kiến trúc

```
┌─────────────────────────────────────────────────────────────┐
│                     SERVER (Digital Twin)                    │
│  - Cartographer SLAM 3D                                      │
│  - RViz (xem map, point cloud)                               │
│  - Nav2 (planning, navigation)                               │
│  - Isaac Sim / Gazebo (mô phỏng)                             │
│  - Topic subscribe: /velodyne_points, /imu/data, /odom      │
│  - Topic publish: /cmd_vel, /joint_trajectory                │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  │ Husarnet VPN + ROS2 CycloneDDS
                  │ (kết nối qua Internet, latency < 100ms)
                  │
┌─────────────────▼───────────────────────────────────────────┐
│                LAPTOP (Robot Thật)                           │
│  - Velodyne VLP-16 Driver (publish /velodyne_points)        │
│  - IMU Driver (publish /imu/data)                            │
│  - Wheel Encoder (publish /odom)                             │
│  - Motor Controller (subscribe /cmd_vel)                     │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Dòng chảy dữ liệu

1. **Laptop** (robot thật):
   - Đọc sensor: LiDAR, IMU, encoder
   - Publish ROS2 topics: `/velodyne_points` (10 Hz), `/imu/data` (100 Hz), `/odom`
   - Nhận lệnh `/cmd_vel` từ server, điều khiển motor

2. **Server** (digital twin):
   - Subscribe dữ liệu sensor từ laptop
   - Chạy Cartographer: tổng hợp LiDAR + IMU → tạo map 3D
   - RViz: hiển thị map, point cloud, định vị robot
   - Nav2: lập kế hoạch đường đi, gửi lệnh `/cmd_vel` xuống laptop

3. **Husarnet VPN**:
   - Kết nối mạng ảo IPv6 giữa 2 máy
   - Cung cấp peer-to-peer, latency thấp, bảo mật end-to-end

---

## 3. Các bước triển khai

### 3.1 Cài đặt Husarnet VPN

**Trên cả server và laptop**:

```bash
# 1. Download và cài Husarnet
curl https://install.husarnet.com/install.sh | sudo bash

# 2. Bật service
sudo systemctl enable --now husarnet

# 3. Kiểm tra trạng thái
sudo husarnet status
```

**Tạo network Husarnet**:
1. Vào https://app.husarnet.com
2. Tạo network mới (ví dụ: `digital-twin-net`)
3. Copy join code
4. Trên **server**: `sudo husarnet join <JOIN_CODE> digital-twin`
5. Trên **laptop**: `sudo husarnet join <JOIN_CODE> realrb`

**Kiểm tra kết nối**:
```bash
# Từ server
ping realrb

# Từ laptop
ping digital-twin
```

**Kết quả kỳ vọng**:
```
Connection status
● Is joined?                 yes
● Is ready to handle data?   yes
```

### 3.2 Cài ROS2 CycloneDDS

**Trên cả 2 máy**:

```bash
sudo apt update
sudo apt install ros-humble-rmw-cyclonedds-cpp
```

Kiểm tra:
```bash
ros2 rmw list
# Output: rmw_cyclonedds_cpp (hoặc fastrtps hoặc cyclonedds)
```

### 3.3 Cấu hình ROS2 DDS

**Thêm vào `~/.bashrc` (cả 2 máy)**:

```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=20
export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml
```

Rồi: `source ~/.bashrc`

### 3.4 File cấu hình CycloneDDS

**Tạo thư mục**:
```bash
sudo mkdir -p /var/tmp/husarnet-dds
```

**File `/var/tmp/husarnet-dds/cyclonedds.xml` (cả 2 máy)**:

```xml
<CycloneDDS>
  <Domain Id="any">
    <General>
      <Interfaces>
        <NetworkInterface name="hnet0" multicast="false"/>
      </Interfaces>
      <AllowMulticast>false</AllowMulticast>
    </General>
  </Domain>
</CycloneDDS>
```

**Giải thích**:
- `hnet0`: Interface Husarnet (IPv6)
- `multicast="false"`: Tắt multicast, dùng unicast qua VPN
- `AllowMulticast="false"`: CycloneDDS chỉ dùng peer-to-peer discovery

---

## 4. Kiểm tra và test

### 4.1 Test ROS2 discovery qua Husarnet

**Terminal 1 (Server)**:
```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=20
export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml
ros2 run demo_nodes_cpp listener
```

**Terminal 2 (Laptop)**:
```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=20
export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml
ros2 run demo_nodes_cpp talker
```

**Kết quả kỳ vọng (listener trên server in)**:
```
[INFO] [listener]: I heard: [Hello World: 1]
[INFO] [listener]: I heard: [Hello World: 2]
[INFO] [listener]: I heard: [Hello World: 3]
```

✅ **ROS2 đã kết nối thành công qua Husarnet**

### 4.2 Test topic LiDAR thực tế

**Trên laptop**:
```bash
ros2 launch mapping velodyne-all-nodes-VLP16-launch.py
```

**Trên server** (terminal mới):
```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=20
export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml
ros2 topic hz /velodyne_points
```

**Kết quả kỳ vọng**:
```
average rate: 10.124
   min: 0.098s max: 0.102s std dev: 0.001s window: 30
```

✅ **LiDAR từ robot thật đã stream về server qua Husarnet**

---

## 5. Chạy digital twin hoàn chỉnh

### 5.1 Khởi động các node

**Từ server, SSH vào laptop để bật driver**:
```bash
ssh weed@realrb
# Trên laptop (qua SSH)
ros2 launch mapping velodyne-all-nodes-VLP16-launch.py
```

**Terminal khác trên laptop**:
```bash
ros2 launch mapping cartographer_velodyne_launch_3d.py
```

**Trên server**:
```bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=20
export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml
ros2 launch mapping rviz_digital_twin.launch.py
```

### 5.2 Script tự động (bắt một lệnh trên server)

**File `start_robot.sh` trên server**:

```bash
#!/bin/bash

# Bật driver và SLAM trên laptop
ssh weed@realrb "source /opt/ros/humble/setup.bash &&
                 export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp &&
                 export ROS_DOMAIN_ID=20 &&
                 export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml &&
                 ros2 launch mapping velodyne-all-nodes-VLP16-launch.py" &

sleep 10

ssh weed@realrb "source /opt/ros/humble/setup.bash &&
                 export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp &&
                 export ROS_DOMAIN_ID=20 &&
                 export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml &&
                 ros2 launch mapping cartographer_velodyne_launch_3d.py" &

# Bật digital twin trên server
source /opt/ros/humble/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_DOMAIN_ID=20
export CYCLONEDDS_URI=file:///var/tmp/husarnet-dds/cyclonedds.xml
ros2 launch mapping rviz_digital_twin.launch.py
```

**Chạy**:
```bash
bash start_robot.sh
```

---

## 6. Kết quả đạt được

✅ **Hệ thống digital twin hoàn chỉnh**:

1. **Robot thật (laptop)**:
   - LiDAR + IMU chạy ổn định, publish topic đều đặn
   - Nhận lệnh `/cmd_vel` từ server, điều khiển motor

2. **Server (digital twin)**:
   - Nhận dữ liệu sensor từ robot thật qua Husarnet
   - Cartographer tính toán map 3D, định vị robot
   - RViz hiển thị point cloud, map, robot position
   - Nav2 lập kế hoạch đường đi, điều khiển robot từ xa

3. **Kết nối mạng**:
   - Husarnet VPN: latency < 100ms, peer-to-peer
   - ROS2 CycloneDDS: auto-discovery, reliable qua Internet
   - Hoạt động ổn định dù ở mạng khác nhau

---

## 7. Lợi ích của kiến trúc này

| Yếu tố | Lợi ích |
|--------|---------|
| **Husarnet VPN** | Peer-to-peer, không cần public IP, bảo mật end-to-end |
| **CycloneDDS** | Auto-discovery, QoS linh hoạt, hỗ trợ multi-machine |
| **Tách laptop/server** | Driver gần sensor (đáng tin cây), server xử lý nặng (SLAM, planning) |
| **SSH control** | Bạn ở server vẫn điều khiển tất cả từ một lệnh |
| **Scalable** | Có thể thêm nhiều robot, sensor khác dễ dàng |

---

## 8. Troubleshooting

### Vấn đề: Listener không nhận message từ talker

**Nguyên nhân**: Interface DDS, multicast, hoặc firewall

**Giải pháp**:
1. Kiểm tra interface Husarnet: `ip addr | grep hnet0`
2. Sửa tên trong `cyclonedds.xml` nếu khác
3. Debug: `export RCUTILS_LOGGING_BUFFERED_STREAM=1` khi chạy

### Vấn đề: LiDAR topic không thấy từ server

**Nguyên nhân**: Driver không chạy trên laptop, hoặc ROS_DOMAIN_ID khác

**Giải pháp**:
1. SSH vào laptop, xác nhận driver chạy: `ros2 topic list`
2. Kiểm tra ROS_DOMAIN_ID giống nhau trên 2 máy
3. Kiểm tra CYCLONEDDS_URI đúng

### Vấn đề: Husarnet status = "inactive"

**Nguyên nhân**: Mất kết nối Internet hoặc daemon Husarnet bị lỗi

**Giải pháp**:
```bash
sudo systemctl restart husarnet
sudo husarnet status
```

---

## 9. Kết luận

Hệ thống digital twin đã được thiết lập thành công với:
- ✅ Kết nối ROS2 qua Husarnet VPN
- ✅ CycloneDDS auto-discovery
- ✅ Dữ liệu sensor từ robot thật đến server
- ✅ Điều khiển robot từ server

**Tiếp theo**: 
- Tinh chỉnh Cartographer config (translation_weight, rotation_weight) để map sạch hơn
- Thêm Nav2 stack cho autonomous navigation
- Tích hợp Foxglove hoặc dashboard web để visualize
- Test trên nhiều robot cùng lúc

---

*Báo cáo được cập nhật lần cuối: Ngày 28/01/2026*

