# NULI Avionics Flight Software
## Setup
### 1. Git/GitHub
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

### 2. Clion
> If you already have CLion installed, skip to Part 3.

Download for your platform from https://www.jetbrains.com/clion/download/. \
You may be asked to create an account. CLion is free for personal use.

### 3. PlatformIO
#### 3.1 PlatformIO Installer Script
> For more information, click [here](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html).

Download for your system using the instructions below.

##### MacOS / Linux
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

##### Windows
1. Download the installer script from [here](https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py)
2. In `Windows PowerShell`, enter the directory where the installer script is located
  1. Use `cd` to navigate directories.
3. Run the script with `python`
  1. `python.exe get-platformio.py`

#### 3.2 CLion PlatformIO Extension
> For more information, click [here](https://www.jetbrains.com/help/clion/platformio.html#install).

Once PlatformIO is installed, install the PlatformIO extension for CLion. Then
restart CLion. \
`File > Settings > Plugins > Marketplace > Search "PlatformIO for CLion"`

#### 3.3 Troubleshooting
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

### 4. Doxygen
Download Doxygen from here: https://www.doxygen.nl/download.html \
Installation instructions: https://www.doxygen.nl/manual/install.html

## Formating Guide
### DO THE FOLLOWING:
- Indentation is 4 spaces
- Opening curley braces same line
- Use c style strings
- Make your class names, variable names, etc descriptive without acronyms
- Use dependency injection when designing classes
- Try to keep function short, to within one screen height when possible
- Code should be easy to read and understand
### AVOID THE FOLLOWING:
- Using "auto" when declaring variables
- Using static variables in functions
- Using dynamic memory on embedded systems
- Using reference variables that persist beyond a function
- Minimize complex object inheritance
### Allowed Acronyms and Abbreviations
- GPS (Global positioning system)
- Num (Number)
- IMU (Inertial measurement unit)
- Pyro (Pyrotechnic Charge)
- 
### Doxygen
Doxygen is a code documentation generator. By formatting comments specifically, 
documentation can be easily generated. The following code block should be placed
before declarations of classes, enums, struts, etc (only the relevant lines).

```cpp
    /**
     * @class ClassName
     * @struct StructName_s
     * @enum EnumName_e
     * @brief A brief description
     * @details A detailed description (optional)
     * @param localVarName Argument description
     * @tparam TTemplateType Template argument description
     * @return Description of return value
     */
```
The following should be placed to the right of the declaration of member variables,
enumerated constants, constants, etc

```cpp
const int NAME = "";    ///< Description
```
### Variables 
- Everything should have descriptive names, without uncommon acronyms or words
- Used sized integer variables like int32_t when possible

```cpp
// Pointers (* on the type)
int32_t* pointerDeclaration;

// Class/struct member variables
int32_t m_classMemberVar = 0;    ///< Description
int32_t structMemberVar = 0;   ///< Description

// Constants
const char* const CONSTANT_NAME = "constantValue";  ///< Description
ENUM_CONSTANT_NAME,     ///< Description

// Local variables (don't require doxygen descriptions)
int32_t useFixedWithIntegers;

```
### Classes
- Class names are UpperCamel, methods are lowerCamel and members are m_lowerCamel
- Everything gets documented with doxygen as described above
- Dependency injection
  - Classes should take member objects as constructor arguments instead of constructing them internally
- Templated classes are OK, but don't get too fancy (no variadic or recursive templates)
- Order is always public/protected/private. Within each the order is data types/constructors/methods/member variables
- All member variables begin with m_
- Separate into .h and .cpp files

```cpp
/**
 * @class ClassName
 * @brief A brief description
 * @details A detailed description (optional)
 * @tparam TTemplateType Template argument description
 */
template<typename TTemplateType>
class ClassName {
public:
    /**
     * @brief A brief description
     * @details A detailed description (optional)
     * @param dependency Argument description
     */
    explicit ClassName(OtherClass* dependency);

    /**
     * @brief A brief description
     * @details A detailed description (optional)
     * @param localVarName Argument description
     * @return Description of return value
     */
    int32_t methodName(double localVarName);

    // Variables go after the methods in each public/protected/private section
    // Public variables are sometimes nice, but are often best hidden behind an API
    int32_t m_publicMemberVar = 0;                              ///< Description

protected:
    // protected methods
    // protected variables

private:
    OtherClass* m_dependency;                                   ///< Description
    double m_memberCamelCase = 0;                               ///< Description
};

// Use .h files for declaring and .cpp files for definitions:
template<typename TTemplateType>
ClassName<TTemplateType>::ClassName(OtherClass* dependency) {
    m_dependency = dependency;
}

template<typename TTemplateType>
int32_t ClassName<TTemplateType>::methodName(double localVarName) {
    return 0;
}
```
### Structs, Enums, Typedefs and Functions
- Use postfix: _s, _e, _t
- Functions are camelCase()

```cpp
/**
 * @struct StructName_s
 * @brief A brief description
 * @details A detailed description (optional)
 */
struct StructName_s {
    double camelCase;                                           ///< Description
    int32_t otherVar;                                           ///< Description
};

/**
 * @enum TypeNameNoPlural_e
 * @brief A brief description
 * @details A detailed description (optional)
 */
enum TypeNameNoPlural_e : uint8_t { // Explicitly control the size
    NAME,                                                       ///< Description
    OTHER_NAME = 5,                                             ///< Description
};

typedef ClassName<StructName_s> TypeName_t;                     ///< Description

// Constants
const char* const CONSTANT_NAME = "constantValue";              ///< Description

void functionName() {
    int32_t useFixedWithIntegers;   // Local variables don't need descriptions, but should have descriptive names
    int32_t* pointerDeclaration;
}
```