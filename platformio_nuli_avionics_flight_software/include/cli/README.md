# Command-Line Interface User Guide
## Flags
There are two types of flags supported which both inherit from `BaseFlag`. 
1. `SimpleFlag`: A typical boolean flag. There is one constructor.
2. `ArgumentFlag`: A flag with one argument. There are two constructors. One with a default argument and one without.
   1. IMPORTANT: Only fundamental types (`int, double, float, etc.`) and `const char*` are allowed.

## Usage
### 0. Include 
```c++
#include "Parser.h" 
```

### 1. Create Parser Object
```c++
Parser myParser = Parser();
```

### 2. Callbacks
Callbacks must have the return type of `void` and must take in two 
arguments. For `SimpleFlag`, a `bool` and a `int8_t` and for `ArgumentFlag`
any fundamental type (plus `const char*`) and a `int8_t`.

```c++
void callback1(bool a, int8_t b);

void callback2(int a, int8_t b) 

void callback3(float a, int8_t b) 

void callback4(uint8_t a, int8_t b) 

void callback5(const char* a, int8_t b);
```

### 2. Declare Flags
```c++
SimpleFlag config("--config", "Configure a trigger with additional flags_s", true, callback1);
ArgumentFlag<int> config_trigger("-t", 0, "Trigger number", true, callback2);
ArgumentFlag<float> config_pulseWidth("-w", 0.0, "Pulse width (required for pwm)", false, callback3);
ArgumentFlag<uint8_t> config_elevation("-e", "Configure ground elevation (in meters)", false, callback4);
ArgumentFlag<const char*> config_notation("-C", "Configuration using expression notation", false, callback5);
```

> Note: While not required by C++, always explicitly signify the type of 
> ArgumentFlag. The compiler can *sometimes* infer the type when given
> a default value, but is finicky and unreliable.

### 3. Add Flags to list of flags
```c++
BaseFlag* configGroup[]{&config, &config_trigger, &config_pulseWidth, &config_elevation};
```

### 4. Add flag list to Parser
> Note: The UID allows the parser to identify which flag group was last inputted.

There are two options:

One, default and use auto-incremented uid from 0
```c++
myParser.addFlagGroup(configGroup);
```
Two, self set uid
```c++
myParser.addFlagGroup(configGroup, 10);
```

### 5. Repeat
Repeat for more groups of flags. 

### 6. Run on input
```c++
myParser.parse(<input>);
```

### 7. Run callbacks
Run the callbacks for the flag group that was most recently added
```c++
myParser.runFlags();
```

### 8. Reset flags for next run
```c++
myParser.resetFlags();
```

## Testing
### CLion
Use the run configurations panel
### Linux
> A shell script is provided under cli > tests > flagTesting.sh which simply packages up the commands listed below.

Build the project with coverage flags enabled:
```shell
rm -rf build
cmake -S . -B build -DENABLE_COVERAGE=ON
cmake --build build
```

Running the project
```shell
cd build
ctest --verbose
```
#### Code Coverage
Generating coverage data 
```shell
lcov --capture --directory . --output-file coverage.info
```

Filter out system files
```shell
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/gtest/*' --output-file coverage_filtered.info
```

Generate HTML report:
```shell
genhtml coverage_filtered.info --output-directory coverage_report --demangle-cpp
```
### Windows/MacOS
Idk.