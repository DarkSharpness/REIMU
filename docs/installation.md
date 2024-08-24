# Installation Details

## Dependencies

### gcc

We need gcc-12 or later to compile the code. To install gcc, run the following command:

```shell
# For Ubuntu/WSL, need to set the default gcc version to 12
sudo apt-get install gcc-12 g++-12
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 60 --slave /usr/bin/g++ g++ /usr/bin/g++-12
# For Arch Linux
sudo pacman -S gcc
```

### xmake

Just follow the official installation guide [here](https://xmake.io/#/guide/installation) to install xmake.

```shell
# Install by curl
curl -fsSL https://xmake.io/shget.text | bash
# For Arch Linux
sudo pacman -Sy xmake
```

### other dependencies

When installing xmake, you may need zip & unzip. To install them, run the following command:

```shell
# For Ubuntu/WSL
sudo apt-get install zip unzip
# For Arch Linux
sudo pacman -S zip unzip
```

## Docker

You can also use Docker to build the project. Just run the following command:

```shell
docker build -t reimu_img .
docker run -it reimu_img
```
