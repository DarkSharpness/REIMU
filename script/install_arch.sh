#!/bin/bash
choose_action() {
    echo "请选择操作:"
    echo "1) 从网络下载"
    echo "2) 本地编译"
    read -p "输入选项 (1 或 2): " choice

    case $choice in
        1)
            echo "正在从网络下载..."
            # If in script directory, just sh
            if [ -f "./from_web.sh" ]; then
                sh from_web.sh
            else
                sh script/from_web.sh
            fi
            ;;
        2)
            echo "正在检查配置文件..."

            gcc_version=$(gcc -dumpversion | cut -d. -f1) 
            if [ $gcc_version -lt 12 ]; then
                echo "gcc 版本过低，正在安装 gcc..."
                sudo pacman -Sy gcc
            fi

            # 检查是否有 xmake
            if ! command -v xmake &> /dev/null; then
                echo "xmake 未安装，正在安装 xmake..."
                sudo pacman -Sy xmake
            fi

            # 检查是否有 zip 和 unzip
            if ! command -v zip &> /dev/null || ! command -v unzip &> /dev/null; then
                echo "zip 或 unzip 未安装，正在安装 zip 和 unzip..."
                sudo pacman -Sy zip unzip
            fi

            # 开始安装
            echo "正在本地编译..."
            xmake f -p linux -a x86_64 -m release
            xmake
            xmake install --admin

            echo "安装完成。"
            ;;
        *)
            echo "无效的选项，请输入 1 或 2。"
            choose_action
            ;;
    esac
}

# 调用函数
choose_action
