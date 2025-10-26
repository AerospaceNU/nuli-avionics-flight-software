#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "Avionics.h"

class Vector3DTransform {
public:
    virtual ~Vector3DTransform() = default;

    virtual Vector3D_s transform(const Vector3D_s &input) const {
        return input;
    }

    static Vector3DTransform identity() {
        return {};
    }
};

#endif //TRANSFORM_H
