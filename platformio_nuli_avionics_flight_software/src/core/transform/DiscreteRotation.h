#ifndef DISCRETE_ROTATION_H
#define DISCRETE_ROTATION_H

#include "Avionics.h"
#include "Vector3DTransform.h"

class DiscreteRotation final : public Vector3DTransform {
public:
    DiscreteRotation() : axis{0, 1, 2}, sign{1, 1, 1} {}

    static DiscreteRotation identity() {
        return DiscreteRotation{};
    }

    Vector3D_s transform(const Vector3D_s& v) const override {
        const float in[3] = {v.x, v.y, v.z};
        return {
                float(sign[0]) * in[axis[0]],
                float(sign[1]) * in[axis[1]],
                float(sign[2]) * in[axis[2]]
            };
    }

    // +90° around X: (x,y,z) -> ( x, -z,  y )
    DiscreteRotation rotateX90() const {
        const DiscreteRotation rot(0, 2, 1, 1, -1, 1);
        return this->compose(rot); // local frame
    }

    // -90° around X: (x,y,z) -> ( x,  z, -y )
    DiscreteRotation rotateXNeg90() const {
        const DiscreteRotation rot(0, 2, 1, 1, 1, -1);
        return this->compose(rot);
    }

    DiscreteRotation rotateY90() const {
        const DiscreteRotation rot(2, 1, 0, 1, 1, -1);
        return this->compose(rot);
    }

    DiscreteRotation rotateYNeg90() const {
        const DiscreteRotation rot(2, 1, 0, -1, 1, 1);
        return this->compose(rot);
    }

    DiscreteRotation rotateZ90() const {
        const DiscreteRotation rot(1, 0, 2, -1, 1, 1);
        return this->compose(rot);
    }

    DiscreteRotation rotateZNeg90() const {
        const DiscreteRotation rot(1, 0, 2, 1, -1, 1);
        return this->compose(rot);
    }

    DiscreteRotation inverse() const {
        DiscreteRotation inv{};
        for (int i = 0; i < 3; ++i) {
            // find which input axis maps to output i
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
    uint8_t axis[3]; // 0=X, 1=Y, 2=Z
    int8_t sign[3]; // +1 or -1

    DiscreteRotation(const uint8_t ax0, const uint8_t ax1, const uint8_t ax2, const int8_t s0, const int8_t s1, const int8_t s2)
        : axis{ax0, ax1, ax2}, sign{s0, s1, s2} {}

    // Internal composition: this ∘ other (apply `other` after `this`)
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
