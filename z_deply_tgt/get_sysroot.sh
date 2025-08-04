#!/bin/bash
# get_sysroot_optimized.sh
# 用法:
#   ./get_sysroot_optimized.sh /path/to/local/sysroot [--with-kernel] [--full]
# 需要目标机配置 sudo 免密码运行 rsync:
#   kylin ALL=(ALL) NOPASSWD: /usr/bin/rsync

TARGET_IP="192.168.2.7"
TARGET_USER="kylin"

echo "目标设备 IP 地址: $TARGET_IP"

if [ -z "$1" ]; then
    echo "错误: 请指定本地 sysroot 路径"
    echo "用法: $0 /path/to/local/sysroot [--with-kernel] [--full]"
    exit 1
fi

LOCAL_SYSROOT="$1"
WITH_KERNEL=false
FULL=false

shift
while (( "$#" )); do
    case "$1" in
        --with-kernel) WITH_KERNEL=true ;;
        --full) FULL=true ;;
        *) echo "未知参数: $1"; exit 1 ;;
    esac
    shift
done

# 检查 SELinux 状态
SELINUX_STATUS=$(ssh ${TARGET_USER}@"$TARGET_IP" "getenforce" 2>/dev/null)
if [ "$SELINUX_STATUS" == "Enforcing" ]; then
    echo "⚠ 警告: 目标机 SELinux 状态为 Enforcing，可能会限制 rsync 访问部分文件"
fi

mkdir -p "$LOCAL_SYSROOT"

echo "开始从 $TARGET_IP 同步 sysroot 到 $LOCAL_SYSROOT ..."

# 公共 rsync 参数（不带引号）
RSYNC_COMMON_OPTS=(
    -avz
    --numeric-ids
    --no-perms
    --no-owner
    --no-group
    --rsync-path=sudo\ rsync
    --exclude=/proc
    --exclude=/sys
    --exclude=/dev
    --exclude=/tmp
    --exclude=/run
    --exclude=/mnt
    --exclude=/media
)

if $FULL; then
    echo "使用 --full 模式，同步完整根目录（排除虚拟目录）"
    rsync "${RSYNC_COMMON_OPTS[@]}" \
        ${TARGET_USER}@"$TARGET_IP":/ "$LOCAL_SYSROOT"
else
    #target machine has /lib as a softlink to /usr/lib, so the DIRS does not contain lib.
    DIRS=("usr/lib" "usr/include" "etc")
    if $WITH_KERNEL; then
        DIRS+=("usr/src")
        echo "启用 --with-kernel，包含 /usr/src"
    fi

    for dir in "${DIRS[@]}"; do
        echo "同步 /$dir ..."
        mkdir -p "$LOCAL_SYSROOT/$(dirname "$dir")"
        rsync "${RSYNC_COMMON_OPTS[@]}" \
            ${TARGET_USER}@"$TARGET_IP":/$dir/ "$LOCAL_SYSROOT/$dir"
        if [ $? -ne 0 ]; then
            echo "错误: 同步 /$dir 失败"
            exit 1
        fi
    done
fi

ln -s $LOCAL_SYSROOT/usr/lib $LOCAL_SYSROOT/lib
ln -s $LOCAL_SYSROOT/usr/lib $LOCAL_SYSROOT/usr/lib64

echo "修复 sysroot 内的符号链接..."
REL_SCRIPT=$(mktemp)
wget -qO "$REL_SCRIPT" https://raw.githubusercontent.com/riscv-collab/riscv-gnu-toolchain/master/sysroot-relativelinks.py
python3 "$REL_SCRIPT" "$LOCAL_SYSROOT"
rm -f "$REL_SCRIPT"

echo "✅ sysroot 同步完成: $LOCAL_SYSROOT"

