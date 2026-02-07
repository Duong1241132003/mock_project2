#!/bin/bash
# Script build và chạy ứng dụng sử dụng CMake

# 1. Tạo thư mục build nếu chưa có
mkdir -p build

# 2. Vào thư mục build
cd build

# 3. Chạy CMake để tạo Makefile (cấu hình dự án)
echo "Dang cau hinh voi CMake..."
cmake ..

# 4. Biên dịch (Build) với tất cả nhân CPU (-j)
echo "Dang bien dich..."
make -j$(nproc)

# 5. Kiểm tra kết quả build và chạy
if [ $? -eq 0 ]; then
    echo "Build thanh cong! Dang copy va khoi dong ung dung..."
    echo "----------------------------------------"
    # Copy binary mới sang thư mục bin
    cp MediaPlayerApp ../bin/
    # Chạy file thực thi từ thư mục bin
    ../bin/MediaPlayerApp
else
    echo "Build THAT BAI! Vui long kiem tra loi phia tren."
fi
