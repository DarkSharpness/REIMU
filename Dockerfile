FROM archlinux:latest
RUN echo 'Server = https://mirror.sjtu.edu.cn/archlinux/$repo/os/$arch' > /etc/pacman.d/mirrorlist \
    && pacman -Sy \
    && pacman -S gcc git xmake unzip zip --noconfirm \
    && cd ~ \
    && git clone https://github.com/DarkSharpness/REIMU.git \
    && cd ~/REIMU \
    && sed -i '/add_requires("fmt")/d' xmake.lua \
    && sed -i '/target("reimu")/a\ add_ldflags("-static")' xmake.lua \
    && export XMAKE_ROOT=y \
    && xmake f -p linux -a x86_64 \
    && xmake \
    && xmake install \
    && rm -rf ~/.cache \
    && rm -rf ~/REIMU
