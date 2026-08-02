"""
Generates TriggerVariables.h/.cpp: the string <-> enum <-> RocketState_s field mapping that
VarExpression uses to bind a trigger condition's variable names (e.g. "altitudeM") to a field of
RocketState_s (see platformio_nuli_avionics_flight_software/src/Avionics.h).

Edit VARIABLES below and re-run (`python generate_trigger_variables.py`) to add, rename, or remove
a variable. TriggerVariables.h/.cpp are generated output - don't hand-edit them, edit this file.
"""

# (notation name used in trigger condition strings, C++ expression reading it off `state`)
VARIABLES = [
    ("runtimeMs", "static_cast<float>(state.timestamp.runtime_ms)"),
    ("flightState", "static_cast<float>(state.flightState)"),
    ("altitudeM", "state.state1D.altitudeM"),
    ("velocityMS", "state.state1D.velocityMS"),
    ("accelerationMSS", "state.state1D.accelerationMSS"),
    ("unfilteredAltitudeM", "state.state1D.unfilteredNoOffsetAltitudeM"),
    ("tiltMagnitudeDeg", "state.orientation.tiltMagnitudeDeg"),
    ("roll", "state.orientation.roll"),
    ("pitch", "state.orientation.pitch"),
    ("yaw", "state.orientation.yaw"),
    ("angularVelocityX", "state.orientation.angularVelocity.x"),
    ("angularVelocityY", "state.orientation.angularVelocity.y"),
    ("angularVelocityZ", "state.orientation.angularVelocity.z"),
    ("positionX", "state.state6D.position.x"),
    ("positionY", "state.state6D.position.y"),
    ("positionZ", "state.state6D.position.z"),
    ("velocityX", "state.state6D.velocity.x"),
    ("velocityY", "state.state6D.velocity.y"),
    ("velocityZ", "state.state6D.velocity.z"),
    ("accelerationX", "state.state6D.acceleration.x"),
    ("accelerationY", "state.state6D.acceleration.y"),
    ("accelerationZ", "state.state6D.acceleration.z"),
    ("gpsAltitudeM", "state.rawGps.altitudeM"),
]

HEADER_PATH = "src/TriggerVariables.h"
SOURCE_PATH = "src/TriggerVariables.cpp"


def enum_name(name):
    return name[0].upper() + name[1:]


with open(HEADER_PATH, "w") as f:
    print("// GENERATED FILE - DO NOT EDIT BY HAND.", file=f)
    print("// Edit generate_trigger_variables.py (in the folder above src/) and re-run it to regenerate.", file=f)
    print("#ifndef TRIGGER_EXPRESSION_PROTOTYPE_TRIGGERVARIABLES_H", file=f)
    print("#define TRIGGER_EXPRESSION_PROTOTYPE_TRIGGERVARIABLES_H", file=f)
    print(file=f)
    print('#include "Avionics.h"', file=f)
    print(file=f)
    print("/**", file=f)
    print(" * @brief Every rocket-state field a trigger condition is allowed to reference by name.", file=f)
    print(" * @details Add entries in generate_trigger_variables.py, not here.", file=f)
    print(" */", file=f)
    print("enum class TriggerVariable_e {", file=f)
    print("    Invalid,", file=f)
    for name, _ in VARIABLES:
        print(f"    {enum_name(name)},", file=f)
    print("};", file=f)
    print(file=f)
    print("/// Looks up the TriggerVariable_e whose notation name exactly matches `name`.", file=f)
    print("TriggerVariable_e getTriggerVariableFromString(const char* name);", file=f)
    print(file=f)
    print("/// Reads the named field out of `state`. Returns 0 for TriggerVariable_e::Invalid.", file=f)
    print("float getTriggerVariableValue(const RocketState_s& state, TriggerVariable_e variable);", file=f)
    print(file=f)
    print("/// Writes the notation name for `variable` into `buffer` (truncated to n, null-terminated).", file=f)
    print("void fillTriggerVariableName(char* buffer, int n, TriggerVariable_e variable);", file=f)
    print(file=f)
    print("#endif //TRIGGER_EXPRESSION_PROTOTYPE_TRIGGERVARIABLES_H", file=f)

with open(SOURCE_PATH, "w") as f:
    print("// GENERATED FILE - DO NOT EDIT BY HAND.", file=f)
    print("// Edit generate_trigger_variables.py (in the folder above src/) and re-run it to regenerate.", file=f)
    print('#include "TriggerVariables.h"', file=f)
    print(file=f)
    print("#include <cstring>", file=f)
    print(file=f)
    print("TriggerVariable_e getTriggerVariableFromString(const char* name) {", file=f)
    for name, _ in VARIABLES:
        print(f'    if (strcmp(name, "{name}") == 0) {{ return TriggerVariable_e::{enum_name(name)}; }}', file=f)
    print("    return TriggerVariable_e::Invalid;", file=f)
    print("}", file=f)
    print(file=f)
    print("float getTriggerVariableValue(const RocketState_s& state, TriggerVariable_e variable) {", file=f)
    print("    switch (variable) {", file=f)
    for name, expr in VARIABLES:
        print(f"        case TriggerVariable_e::{enum_name(name)}: return {expr};", file=f)
    print("        case TriggerVariable_e::Invalid: default: return 0.0f;", file=f)
    print("    }", file=f)
    print("}", file=f)
    print(file=f)
    print("void fillTriggerVariableName(char* buffer, int n, TriggerVariable_e variable) {", file=f)
    print('    const char* name = "invalid";', file=f)
    print("    switch (variable) {", file=f)
    for name, _ in VARIABLES:
        print(f'        case TriggerVariable_e::{enum_name(name)}: name = "{name}"; break;', file=f)
    print("        case TriggerVariable_e::Invalid: default: break;", file=f)
    print("    }", file=f)
    print("    strncpy(buffer, name, n);", file=f)
    print("    if (n > 0) buffer[n - 1] = '\\0';", file=f)
    print("}", file=f)

print(f"Wrote {HEADER_PATH} and {SOURCE_PATH} ({len(VARIABLES)} variables).")
