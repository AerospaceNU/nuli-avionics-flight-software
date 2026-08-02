// GENERATED FILE - DO NOT EDIT BY HAND.
// Edit generate_trigger_variables.py (in the folder above src/) and re-run it to regenerate.
#ifndef TRIGGER_EXPRESSION_PROTOTYPE_TRIGGERVARIABLES_H
#define TRIGGER_EXPRESSION_PROTOTYPE_TRIGGERVARIABLES_H

#include "Avionics.h"

/**
 * @brief Every rocket-state field a trigger condition is allowed to reference by name.
 * @details Add entries in generate_trigger_variables.py, not here.
 */
enum class TriggerVariable_e {
    Invalid,
    RuntimeMs,
    FlightState,
    AltitudeM,
    VelocityMS,
    AccelerationMSS,
    UnfilteredAltitudeM,
    TiltMagnitudeDeg,
    Roll,
    Pitch,
    Yaw,
    AngularVelocityX,
    AngularVelocityY,
    AngularVelocityZ,
    PositionX,
    PositionY,
    PositionZ,
    VelocityX,
    VelocityY,
    VelocityZ,
    AccelerationX,
    AccelerationY,
    AccelerationZ,
    GpsAltitudeM,
};

/// Looks up the TriggerVariable_e whose notation name exactly matches `name`.
TriggerVariable_e getTriggerVariableFromString(const char* name);

/// Reads the named field out of `state`. Returns 0 for TriggerVariable_e::Invalid.
float getTriggerVariableValue(const RocketState_s& state, TriggerVariable_e variable);

/// Writes the notation name for `variable` into `buffer` (truncated to n, null-terminated).
void fillTriggerVariableName(char* buffer, int n, TriggerVariable_e variable);

#endif //TRIGGER_EXPRESSION_PROTOTYPE_TRIGGERVARIABLES_H
