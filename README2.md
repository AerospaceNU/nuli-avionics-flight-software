# Intro to Avionics Setup Guide
## 1. Git/GitHub
> This assumes you already have `git` installed. If not, see 
> [this](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git) page
> for installation options.

> If you already have an SSH key added to GitHub, skip to Part 2.

All of our code is hosted in GitHub.
> When cloning a GitHub repository, use the `SSH` option, and not `HTTPS`. 
> `HTTPS` requires setting up a GitHub passphrase, which is fine, but just
> requires additional setup.

1. Generate your SSH key. When prompted, just use the default location. \
`ssh-keygen -t ed25519 -C "comment"`
2. Copy the **public** key. A file ending in `.pub` likely named
`id_ed25519.pub`. \
`cat ~/.ssh/id_ed25519.pub`
3. Add your key to GitHub \
`GitHub profile picture > Settings > SSH and GPG Keys > New SSH key`

## 2. Clion
> If you already have CLion installed, skip to Part 3.

Download for your platform from https://www.jetbrains.com/clion/download/. \
You may be asked to create an account. CLion is free for personal use.

## 3. PlatformIO
### 3.1 PlatformIO Installer Script
> For more information, click [here](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html).

Download for your system using the instructions below.

#### MacOS / Linux
Using `curl`
```shell
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

Using `wget`
```shell
wget -O get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

#### Windows
1. Download the installer script from [here](https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py)
2. In `Windows PowerShell`, enter the directory where the installer script is located
   1. Use `cd` to navigate directories.
3. Run the script with `python`
   1. `python.exe get-platformio.py`

### 3.2 CLion PlatformIO Extension
Once PlatformIO is installed, install the PlatformIO extension for CLion. Then
restart CLion. \
`File > Settings > Plugins > Marketplace > Search "PlatformIO for CLion"`
> For more information, click [here](https://www.jetbrains.com/help/clion/platformio.html#install).

### 3.3 Troubleshooting
Sometimes, PlatformIO likes to fail to load or build when starting from an 
existing project. To solve this, we will load a new ProjectIO project from
on top of our existing project.

1. Create a new project from existing source (e.g. nuli-avionics-flight-software).
   1. `File > New > Project > PlatformIO in the sidebar > Adafruit Feather M0 (SAMD21G18A)`.
   2. Under `location` choose the directory containing the existing project.
2. Delete generated files and folders
   1. `.pio/`
   2. `src/main.cpp`
   3. The `[env:adafruit...]` section in `platformio.ini`
3. Reload the PlatformIO project
   1. `Tools > PlatformIO > Reload PlatformIO Project`
4. Build project
   1. Click the `Hammer` icon. If everything builds correctly, fantastic!

## 4. Doxygen
Download Doxygen from here: https://www.doxygen.nl/download.html \
Installation instructions: https://www.doxygen.nl/manual/install.html

