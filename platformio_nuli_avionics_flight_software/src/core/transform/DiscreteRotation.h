#ifndef DISCRETE_ROTATION_H
#define DISCRETE_ROTATION_H

#include "Avionics.h"
#include "Vector3DTransform.h"

/**
 * @class DiscreteRotation
 * @brief Represents a 3D rotation composed only of 90° increments around the X, Y, or Z axes.
 * @details Stores the mapping of input axes to output axes and their signs, to efficiently compose, invert, and apply axis-aligned rotations.
 */
class DiscreteRotation final : public Vector3DTransform {
public:
    /** @brief Constructs an identity rotation: X->X, Y->Y, Z->Z, all positive directions. */
    DiscreteRotation() : axis{0, 1, 2}, sign{1, 1, 1} {}

    /**
     * @brief Returns an identity rotation.
     * @details Equivalent to the default constructor - does not alter any input vector.
     * @return A DiscreteRotation representing no rotation.
     */
    static DiscreteRotation identity() {
        return DiscreteRotation{};
    }

    /**
     * @brief Applies this rotation to a 3D vector.
     * @details Permutes and sign-flips the input vector's components per the rotation definition.
     * @param v Input vector to transform.
     * @return The transformed vector after applying the rotation.
     */
    Vector3D_s transform(const Vector3D_s& v) const override {
        // sign[i] is always +-1, so a conditional negate is exact and avoids a soft-float multiply
        // (plus the int->float conversion it required) per axis on FPU-less parts.
        const float in[3] = {v.x, v.y, v.z};
        return {
                sign[0] < 0 ? -in[axis[0]] : in[axis[0]],
                sign[1] < 0 ? -in[axis[1]] : in[axis[1]],
                sign[2] < 0 ? -in[axis[2]] : in[axis[2]]
            };
    }

    /**
     * @brief Returns a new rotation rotated +90° about the local X-axis.
     * @return A new DiscreteRotation representing this local rotation.
     */
    DiscreteRotation rotateX90local() const {
        const DiscreteRotation rot(0, 2, 1, 1, -1, 1);
        return this->compose(rot); // local frame
    }

    /**
     * @brief Returns a new rotation rotated -90° about the local X-axis.
     * @return A new DiscreteRotation representing this local rotation.
     */
    DiscreteRotation rotateXNeg90local() const {
        const DiscreteRotation rot(0, 2, 1, 1, 1, -1);
        return this->compose(rot);
    }

    /**
     * @brief Returns a new rotation rotated +90° about the local Y-axis.
     * @return A new DiscreteRotation representing this local rotation.
     */
    DiscreteRotation rotateY90local() const {
        const DiscreteRotation rot(2, 1, 0, 1, 1, -1);
        return this->compose(rot);
    }

    /**
     * @brief Returns a new rotation rotated -90° about the local Y-axis.
     * @return A new DiscreteRotation representing this local rotation.
     */
    DiscreteRotation rotateYNeg90local() const {
        const DiscreteRotation rot(2, 1, 0, -1, 1, 1);
        return this->compose(rot);
    }

    /**
     * @brief Returns a new rotation rotated +90° about the local Z-axis.
     * @return A new DiscreteRotation representing this local rotation.
     */
    DiscreteRotation rotateZ90local() const {
        const DiscreteRotation rot(1, 0, 2, -1, 1, 1);
        return this->compose(rot);
    }

    /**
     * @brief Returns a new rotation rotated -90° about the local Z-axis.
     * @return A new DiscreteRotation representing this local rotation.
     */
    DiscreteRotation rotateZNeg90local() const {
        const DiscreteRotation rot(1, 0, 2, 1, -1, 1);
        return this->compose(rot);
    }

    /**
     * @brief Returns the inverse of this rotation.
     * @details Applying this rotation and its inverse consecutively yields the identity rotation.
     * @return A DiscreteRotation representing the inverse rotation.
     */
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
    uint8_t axis[3]; ///< Axis mapping: 0 = X, 1 = Y, 2 = Z.
    int8_t sign[3];  ///< Sign mapping: +1 or -1 for each axis direction.

    /**
     * @brief Constructs a rotation from explicit axis mappings and signs.
     * @details Used internally to define specific 90° rotations around axes.
     * @param ax0 Output axis index for input X.
     * @param ax1 Output axis index for input Y.
     * @param ax2 Output axis index for input Z.
     * @param s0 Sign for output X component.
     * @param s1 Sign for output Y component.
     * @param s2 Sign for output Z component.
     */
    DiscreteRotation(const uint8_t ax0, const uint8_t ax1, const uint8_t ax2,
                     const int8_t s0, const int8_t s1, const int8_t s2)
        : axis{ax0, ax1, ax2}, sign{s0, s1, s2} {}

    /**
     * @brief Composes this rotation with another.
     * @details Returns the result of applying `other` after this rotation, combining the two transformations in sequence.
     * @param other The rotation to apply after this rotation.
     * @return A new DiscreteRotation representing the composed rotation.
     */
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
