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
Installer Script: https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html \
CLion PlatformIO instructions: https://www.jetbrains.com/help/clion/platformio.html#install

Follow the instructions linked above to download PlatformIO for your system.

Once PlatformIO is installed, install the PlatformIO extension for CLion. Then
restart CLion. \
`File > Settings > Plugins > Marketplace > Search "PlatformIO for CLion"`

### Troubleshooting
Sometimes, PlatformIO likes to fail when building from an existing project.
This could be the result of anything from ... To solve this, we will create a
new ProjectIO project from scratch.

1. Create a new project, name this the same as the original ProjectIO folder 
(e.g. nuli-avionics-flight-software)
   1. `File > New > Project > PlatformIO in the sidebar > Adafruit Feather M0 (SAMD21G18A)`
2. Copy original source files into new project
   1. `include`, `src`, `test`
3. Copy newly created project back into the original directory

## 4. Doxygen
Download Doxygen from here: https://www.doxygen.nl/download.html \
Installation instructions: https://www.doxygen.nl/manual/install.html

