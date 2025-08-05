#!/bin/sh
# GPIO 初始化脚本
# 目标：提前 export GPIO115/146/147，设置方向/边沿，并修改权限
# 作者：Mr.Flying 定制

set -e  # 出错立即退出

# 115: 灯光控制（输出）
if [ ! -d /sys/class/gpio/gpio115 ]; then
    echo 115 > /sys/class/gpio/export
fi
echo out > /sys/class/gpio/gpio115/direction
chmod 666 /sys/class/gpio/gpio115/value

# 146: 左键（输入，双边沿触发）
if [ ! -d /sys/class/gpio/gpio146 ]; then
    echo 146 > /sys/class/gpio/export
fi
echo in > /sys/class/gpio/gpio146/direction
echo both > /sys/class/gpio/gpio146/edge
chmod 666 /sys/class/gpio/gpio146/value

# 147: 右键（输入，双边沿触发）
if [ ! -d /sys/class/gpio/gpio147 ]; then
    echo 147 > /sys/class/gpio/export
fi
echo in > /sys/class/gpio/gpio147/direction
echo both > /sys/class/gpio/gpio147/edge
chmod 666 /sys/class/gpio/gpio147/value

echo "[GPIO INIT] 完成 GPIO115/146/147 初始化"

