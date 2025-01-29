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

### 3. Add Flags to FlagGroup
```c++
BaseFlag* group1[]{&config, &config_trigger, &config_pulseWidth, &config_elevation};
Parser::FlagGroup configGroup(group1);
```

### 4. Add FlagGroup to Parser
```c++
myParser.addFlagGroup(configGroup);
```

### 5. Repeat

Repeat for more FlagGroups. 
