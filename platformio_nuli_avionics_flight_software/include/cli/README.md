# Command-Line Interface User Guide
## Flags
There are two types of flags supported which both inherit from `BaseFlag`. 
1. `SimpleFlag`: A typical boolean flag. There is one constructor.
2. `ArgumentFlag`: A flag with one argument. There are two constructors. One with a default argument and one without.

## Usage
### 1. Create Parser Object
```c++
Parser myParser = Parser();
```

### 2. Declare Flags
```c++
SimpleFlag config("--config", "Configure a trigger with additional flags_s", true);
ArgumentFlag<int> config_trigger("-t", 0, "Trigger number", true);
ArgumentFlag<float> config_pulseWidth("-w", 0.0, "Pulse width (required for pwm)", false);
ArgumentFlag<uint8_t> config_elevation("-e", "Configure ground elevation (in meters)", false);
```

> Note: While not required by C++, always explicitly signify the type of 
> ArgumentFlag. While the compiler can *sometimes* infer the type when given
> a default value, this is finicky and unreliable.

### 3. Add Flags to list of flags
```c++
BaseFlag* configGroup[]{&config, &config_trigger, &config_pulseWidth, &config_elevation};
```

> Note: naming preference should be `[leaderFlagName]Group` for readability.

### 4. Add flag list to Parser
```c++
myParser.addFlagGroup(configGroup);
```

### 5. Repeat
Repeat for more groups of flags. 

## Testing
### CLion
Use the run configurations panel
### Linux
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