# Hệ thống Giám sát sức khỏe kết hợp Android<br>
Chế tạo thiết bị di động nhỏ gọn giúp đo nhịp tim, nồng độ oxy (SpO2), nhiệt độ cơ thể và hiển thị trực quan lên màn hình OLED 0.96 inch.<br>
📋 Giới thiệu dự án<br>
Dự án được thiết kế để hỗ trợ giám sát sức khỏe cá nhân, tích hợp cảm biến đo nhịp tim, nồng độ oxy trong máu (SpO2) và nhiệt độ cơ thể. Dữ liệu được xử lý tại chỗ và hiển thị trực quan thông qua màn hình OLED, đồng thời được lưu trữ trên Firebase để theo dõi từ xa qua ứng dụng Android.<br>
🛠 Công nghệ & Linh kiện<br>
Vi điều khiển: ESP32.<br>
Cảm biến: MAX30102 (nhịp tim, SpO2), DS18B20 (nhiệt độ).<br>
Hiển thị: Màn hình OLED 0.96 inch.  <br>
Nguồn: Pin Lithium 18650, mạch sạc TP4056, mạch Boost DC-DC MT3608.<br>
Phần mềm: Android Studio (Java), Firebase Realtime Database.  <br>
⚙️ Kỹ năng kỹ thuật đã áp dụng<br>
Thiết kế & Thi công: Tự tay vẽ layout và thi công mạch in (PCB) thủ công.<br>
Kỹ năng hàn: Hàn thành thạo linh kiện cắm (DIP), đảm bảo độ ổn định cho mạch nguồn và hệ thống.<br>
Truyền thông: Lập trình giao tiếp với cảm biến qua chuẩn I2C và 1-Wire.<br>
IoT & Cloud: Đồng bộ dữ liệu theo thời gian thực (Realtime) với Firebase.<br>
🖼 Hình ảnh dự án<br>
Sơ đồ kết nối dây:<br>
<img width="1352" height="461" alt="image" src="https://github.com/user-attachments/assets/45be719b-24b4-4d56-b6a2-4e5d71ed1650" />
<img width="672" height="630" alt="image" src="https://github.com/user-attachments/assets/2b5ae7d4-6c76-4457-b159-57a12b85bbae" />
<img width="783" height="550" alt="image" src="https://github.com/user-attachments/assets/9031dff3-fe93-467e-a491-f379a55b9a82" />
<img width="963" height="623" alt="image" src="https://github.com/user-attachments/assets/081186ea-7aa7-4241-9e7e-374b6c311683" />
<img width="608" height="629" alt="image" src="https://github.com/user-attachments/assets/5dd22228-3625-497f-96ec-9ee8b93302bf" />
<img width="733" height="685" alt="image" src="https://github.com/user-attachments/assets/4ea1eeb8-a9ac-4067-9653-f8a0b7565d69" />
<img width="562" height="875" alt="image" src="https://github.com/user-attachments/assets/1a9e86e5-5288-464a-8c1f-428d7943318d" />
<img width="412" height="521" alt="image" src="https://github.com/user-attachments/assets/bb058fd6-a25a-4764-b147-3db3f808d2c5" />
Phần mềm:
<img width="378" height="802" alt="image" src="https://github.com/user-attachments/assets/5d15db38-37c7-4b49-b560-42a9e24f4d3a" />
<img width="386" height="808" alt="image" src="https://github.com/user-attachments/assets/deaf4003-f7fa-4017-b38c-8419733d6162" />
<img width="374" height="799" alt="image" src="https://github.com/user-attachments/assets/4fb2bbef-3fc6-4271-b46b-cbb49914f748" />
<img width="395" height="819" alt="image" src="https://github.com/user-attachments/assets/3200ccee-7ed9-4b52-896c-74be484f0af3" />

