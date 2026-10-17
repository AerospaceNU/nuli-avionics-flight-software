#ifndef DISCRETE_ROTATION_H
#define DISCRETE_ROTATION_H

#include "Avionics.h"
#include "Vector3DTransform.h"

/**
 * @brief 3D rotation composed only of 90° steps about X, Y or Z, stored as an axis permutation plus signs.
 */
class DiscreteRotation final : public Vector3DTransform {
public:
    DiscreteRotation() : axis{0, 1, 2}, sign{1, 1, 1} {}

    static DiscreteRotation identity() {
        return DiscreteRotation{};
    }

    Vector3D_s transform(const Vector3D_s& v) const override {
        // sign[i] is always +-1: conditional negate avoids a soft-float multiply on FPU-less parts
        const float in[3] = {v.x, v.y, v.z};
        return {
                sign[0] < 0 ? -in[axis[0]] : in[axis[0]],
                sign[1] < 0 ? -in[axis[1]] : in[axis[1]],
                sign[2] < 0 ? -in[axis[2]] : in[axis[2]]
            };
    }

    /** @brief Returns this rotation followed by +90° about the local X axis. */
    DiscreteRotation rotateX90local() const {
        const DiscreteRotation rot(0, 2, 1, 1, -1, 1);
        return this->compose(rot);
    }

    /** @brief Returns this rotation followed by -90° about the local X axis. */
    DiscreteRotation rotateXNeg90local() const {
        const DiscreteRotation rot(0, 2, 1, 1, 1, -1);
        return this->compose(rot);
    }

    /** @brief Returns this rotation followed by +90° about the local Y axis. */
    DiscreteRotation rotateY90local() const {
        const DiscreteRotation rot(2, 1, 0, 1, 1, -1);
        return this->compose(rot);
    }

    /** @brief Returns this rotation followed by -90° about the local Y axis. */
    DiscreteRotation rotateYNeg90local() const {
        const DiscreteRotation rot(2, 1, 0, -1, 1, 1);
        return this->compose(rot);
    }

    /** @brief Returns this rotation followed by +90° about the local Z axis. */
    DiscreteRotation rotateZ90local() const {
        const DiscreteRotation rot(1, 0, 2, -1, 1, 1);
        return this->compose(rot);
    }

    /** @brief Returns this rotation followed by -90° about the local Z axis. */
    DiscreteRotation rotateZNeg90local() const {
        const DiscreteRotation rot(1, 0, 2, 1, -1, 1);
        return this->compose(rot);
    }

    DiscreteRotation inverse() const {
        DiscreteRotation inv{};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (axis[j] == i) {
                    inv.axis[i] = j;
                    inv.sign[i] = sign[j];
                }
            }
        }
        return inv;
    }

private:
    uint8_t axis[3]; ///< Source axis for each output component: 0 = X, 1 = Y, 2 = Z.
    int8_t sign[3];  ///< +1 or -1 for each output component.

    DiscreteRotation(const uint8_t ax0, const uint8_t ax1, const uint8_t ax2,
                     const int8_t s0, const int8_t s1, const int8_t s2)
        : axis{ax0, ax1, ax2}, sign{s0, s1, s2} {}

    /** @brief Returns `other` applied in this rotation's local frame. */
    DiscreteRotation compose(const DiscreteRotation& other) const {
        DiscreteRotation r{};
        for (int i = 0; i < 3; ++i) {
            r.axis[i] = other.axis[axis[i]];
            r.sign[i] = sign[i] * other.sign[axis[i]];
        }
        return r;
    }
};

#endif //DISCRETE_ROTATION_H
