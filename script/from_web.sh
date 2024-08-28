#!/bin/bash

# GitHub 仓库信息
OWNER="DarkSharpness"
REPO="REIMU"

# 获取最新 release 的下载链接
LATEST_RELEASE_URL=$(curl -s "https://api.github.com/repos/$OWNER/$REPO/releases/latest" | grep "browser_download_url" | cut -d '"' -f 4)
LATEST_RELEASE_URL=$(echo "$LATEST_RELEASE_URL" | grep "linux-x86_64")

if [ -z "$LATEST_RELEASE_URL" ]; then
    echo "无法获取最新的 release 下载链接。"
    exit 1
fi

echo $LATEST_RELEASE_URL

# 下载 binary 到 /tmp
curl -L -o /tmp/reimu "$LATEST_RELEASE_URL"

if [ $? -ne 0 ]; then
    echo "下载失败。"
    exit 1
fi

# 移动到 /usr/local/bin
sudo mv /tmp/reimu /usr/local/bin/reimu

# 删除 /tmp 下的文件
rm /tmp/reimu
