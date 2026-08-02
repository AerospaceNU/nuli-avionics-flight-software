// GENERATED FILE - DO NOT EDIT BY HAND.
// Edit generate_trigger_variables.py (in the folder above src/) and re-run it to regenerate.
#include "TriggerVariables.h"

#include <cstring>

TriggerVariable_e getTriggerVariableFromString(const char* name) {
    if (strcmp(name, "runtimeMs") == 0) { return TriggerVariable_e::RuntimeMs; }
    if (strcmp(name, "flightState") == 0) { return TriggerVariable_e::FlightState; }
    if (strcmp(name, "altitudeM") == 0) { return TriggerVariable_e::AltitudeM; }
    if (strcmp(name, "velocityMS") == 0) { return TriggerVariable_e::VelocityMS; }
    if (strcmp(name, "accelerationMSS") == 0) { return TriggerVariable_e::AccelerationMSS; }
    if (strcmp(name, "unfilteredAltitudeM") == 0) { return TriggerVariable_e::UnfilteredAltitudeM; }
    if (strcmp(name, "tiltMagnitudeDeg") == 0) { return TriggerVariable_e::TiltMagnitudeDeg; }
    if (strcmp(name, "roll") == 0) { return TriggerVariable_e::Roll; }
    if (strcmp(name, "pitch") == 0) { return TriggerVariable_e::Pitch; }
    if (strcmp(name, "yaw") == 0) { return TriggerVariable_e::Yaw; }
    if (strcmp(name, "angularVelocityX") == 0) { return TriggerVariable_e::AngularVelocityX; }
    if (strcmp(name, "angularVelocityY") == 0) { return TriggerVariable_e::AngularVelocityY; }
    if (strcmp(name, "angularVelocityZ") == 0) { return TriggerVariable_e::AngularVelocityZ; }
    if (strcmp(name, "positionX") == 0) { return TriggerVariable_e::PositionX; }
    if (strcmp(name, "positionY") == 0) { return TriggerVariable_e::PositionY; }
    if (strcmp(name, "positionZ") == 0) { return TriggerVariable_e::PositionZ; }
    if (strcmp(name, "velocityX") == 0) { return TriggerVariable_e::VelocityX; }
    if (strcmp(name, "velocityY") == 0) { return TriggerVariable_e::VelocityY; }
    if (strcmp(name, "velocityZ") == 0) { return TriggerVariable_e::VelocityZ; }
    if (strcmp(name, "accelerationX") == 0) { return TriggerVariable_e::AccelerationX; }
    if (strcmp(name, "accelerationY") == 0) { return TriggerVariable_e::AccelerationY; }
    if (strcmp(name, "accelerationZ") == 0) { return TriggerVariable_e::AccelerationZ; }
    if (strcmp(name, "gpsAltitudeM") == 0) { return TriggerVariable_e::GpsAltitudeM; }
    return TriggerVariable_e::Invalid;
}

float getTriggerVariableValue(const RocketState_s& state, TriggerVariable_e variable) {
    switch (variable) {
        case TriggerVariable_e::RuntimeMs: return static_cast<float>(state.timestamp.runtime_ms);
        case TriggerVariable_e::FlightState: return static_cast<float>(state.flightState);
        case TriggerVariable_e::AltitudeM: return state.state1D.altitudeM;
        case TriggerVariable_e::VelocityMS: return state.state1D.velocityMS;
        case TriggerVariable_e::AccelerationMSS: return state.state1D.accelerationMSS;
        case TriggerVariable_e::UnfilteredAltitudeM: return state.state1D.unfilteredNoOffsetAltitudeM;
        case TriggerVariable_e::TiltMagnitudeDeg: return state.orientation.tiltMagnitudeDeg;
        case TriggerVariable_e::Roll: return state.orientation.roll;
        case TriggerVariable_e::Pitch: return state.orientation.pitch;
        case TriggerVariable_e::Yaw: return state.orientation.yaw;
        case TriggerVariable_e::AngularVelocityX: return state.orientation.angularVelocity.x;
        case TriggerVariable_e::AngularVelocityY: return state.orientation.angularVelocity.y;
        case TriggerVariable_e::AngularVelocityZ: return state.orientation.angularVelocity.z;
        case TriggerVariable_e::PositionX: return state.state6D.position.x;
        case TriggerVariable_e::PositionY: return state.state6D.position.y;
        case TriggerVariable_e::PositionZ: return state.state6D.position.z;
        case TriggerVariable_e::VelocityX: return state.state6D.velocity.x;
        case TriggerVariable_e::VelocityY: return state.state6D.velocity.y;
        case TriggerVariable_e::VelocityZ: return state.state6D.velocity.z;
        case TriggerVariable_e::AccelerationX: return state.state6D.acceleration.x;
        case TriggerVariable_e::AccelerationY: return state.state6D.acceleration.y;
        case TriggerVariable_e::AccelerationZ: return state.state6D.acceleration.z;
        case TriggerVariable_e::GpsAltitudeM: return state.rawGps.altitudeM;
        case TriggerVariable_e::Invalid: default: return 0.0f;
    }
}

void fillTriggerVariableName(char* buffer, int n, TriggerVariable_e variable) {
    const char* name = "invalid";
    switch (variable) {
        case TriggerVariable_e::RuntimeMs: name = "runtimeMs"; break;
        case TriggerVariable_e::FlightState: name = "flightState"; break;
        case TriggerVariable_e::AltitudeM: name = "altitudeM"; break;
        case TriggerVariable_e::VelocityMS: name = "velocityMS"; break;
        case TriggerVariable_e::AccelerationMSS: name = "accelerationMSS"; break;
        case TriggerVariable_e::UnfilteredAltitudeM: name = "unfilteredAltitudeM"; break;
        case TriggerVariable_e::TiltMagnitudeDeg: name = "tiltMagnitudeDeg"; break;
        case TriggerVariable_e::Roll: name = "roll"; break;
        case TriggerVariable_e::Pitch: name = "pitch"; break;
        case TriggerVariable_e::Yaw: name = "yaw"; break;
        case TriggerVariable_e::AngularVelocityX: name = "angularVelocityX"; break;
        case TriggerVariable_e::AngularVelocityY: name = "angularVelocityY"; break;
        case TriggerVariable_e::AngularVelocityZ: name = "angularVelocityZ"; break;
        case TriggerVariable_e::PositionX: name = "positionX"; break;
        case TriggerVariable_e::PositionY: name = "positionY"; break;
        case TriggerVariable_e::PositionZ: name = "positionZ"; break;
        case TriggerVariable_e::VelocityX: name = "velocityX"; break;
        case TriggerVariable_e::VelocityY: name = "velocityY"; break;
        case TriggerVariable_e::VelocityZ: name = "velocityZ"; break;
        case TriggerVariable_e::AccelerationX: name = "accelerationX"; break;
        case TriggerVariable_e::AccelerationY: name = "accelerationY"; break;
        case TriggerVariable_e::AccelerationZ: name = "accelerationZ"; break;
        case TriggerVariable_e::GpsAltitudeM: name = "gpsAltitudeM"; break;
        case TriggerVariable_e::Invalid: default: break;
    }
    strncpy(buffer, name, n);
    if (n > 0) buffer[n - 1] = '\0';
}
