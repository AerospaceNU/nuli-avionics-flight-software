#ifndef ROTATIONHELO_H
#define ROTATIONHELO_H

#include "Avionics.h"
#include <cmath>
#include "Vector3DTransform.h"

class QuaternionHelper final {
public:
    // Conjugate (inverse for unit Quaternion2)
    static Quaternion_s conjugate(const Quaternion_s& q) {
        return {-q.x, -q.y, -q.z, q.w};
    }

    // Quaternion2 multiplication (this * rhs)
    static Quaternion_s multiply(const Quaternion_s& q, const Quaternion_s& rhs) {
        return {
                q.w * rhs.x + q.x * rhs.w + q.y * rhs.z - q.z * rhs.y,
                q.w * rhs.y - q.x * rhs.z + q.y * rhs.w + q.z * rhs.x,
                q.w * rhs.z + q.x * rhs.y - q.y * rhs.x + q.z * rhs.w,
                q.w * rhs.w - q.x * rhs.x - q.y * rhs.y - q.z * rhs.z
            };
    }

    // Rotate a 3D vector using this Quaternion2
    static Vector3D_s rotateVector(const Quaternion_s& q, const Vector3D_s& v) {
        const Vector3D_s r = {q.x, q.y, q.z}; // vector part of Quaternion2
        const float s = q.w; // scalar part

        // v' = v + 2 * cross(r, s*v + cross(r, v))
        const Vector3D_s cross1 = {
                r.y * v.z - r.z * v.y,
                r.z * v.x - r.x * v.z,
                r.x * v.y - r.y * v.x
            };

        const Vector3D_s t = {
                s * v.x + cross1.x,
                s * v.y + cross1.y,
                s * v.z + cross1.z
            };

        const Vector3D_s cross2 = {
                r.y * t.z - r.z * t.y,
                r.z * t.x - r.x * t.z,
                r.x * t.y - r.y * t.x
            };

        return {
                v.x + 2.0f * cross2.x,
                v.y + 2.0f * cross2.y,
                v.z + 2.0f * cross2.z
            };
    }

    // Quaternion2 magnitude
    static float norm(const Quaternion_s& q) {
        return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    }

    // Normalize in place
    static void normalize(Quaternion_s& q) {
        const float n = norm(q);
        if (n > 0.0f) {
            const float inv = 1.0f / n;
            q.x *= inv;
            q.y *= inv;
            q.z *= inv;
            q.w *= inv;
        }
    }

    static Vector3D_s toAxisAngleVector(const Quaternion_s& q) {
        Quaternion_s qInternal = q;
        normalize(qInternal);

        const float angle = 2.0f * std::acos(qInternal.w);
        const float s = std::sqrt(1.0f - qInternal.w * qInternal.w); // assuming unit Quaternion2

        Vector3D_s axis{};
        if (s < 1e-6f) {
            // Axis is not well-defined, pick X axis
            axis = {1.0f, 0.0f, 0.0f};
        } else {
            axis = {qInternal.x / s, qInternal.y / s, qInternal.z / s};
        }

        // Axis * angle gives a compact 3D representation
        return {axis.x * angle, axis.y * angle, axis.z * angle};
    }

    static Vector3D_s toEulerRPY(const Quaternion_s& q) {
        Quaternion_s qInternal = q;
        normalize(qInternal);

        // roll (x-axis rotation)
        const float sinr_cosp = 2.0f * (qInternal.w * qInternal.x + qInternal.y * qInternal.z);
        const float cosr_cosp = 1.0f - 2.0f * (qInternal.x * qInternal.x + qInternal.y * qInternal.y);
        const float roll = std::atan2(sinr_cosp, cosr_cosp);

        // pitch (y-axis rotation)
        const float sinp = 2.0f * (qInternal.w * qInternal.y - qInternal.z * qInternal.x);
        float pitch;
        if (std::fabs(sinp) >= 1.0f) {
            pitch = std::copysign(static_cast<float>(M_PI) / 2.0f, sinp);
        } else {
            pitch = std::asin(sinp);
        }

        // yaw (z-axis rotation)
        const float siny_cosp = 2.0f * (qInternal.w * qInternal.z + qInternal.x * qInternal.y);
        const float cosy_cosp = 1.0f - 2.0f * (qInternal.y * qInternal.y + qInternal.z * qInternal.z);
        const float yaw = std::atan2(siny_cosp, cosy_cosp);

        return {roll, pitch, yaw};
    }

    static Quaternion_s fromGravityVector(const Vector3D_s& accel_body) {
        // Normalize accelerometer vector
        Vector3D_s b = accel_body;
        const float bnorm = std::sqrt(b.x*b.x + b.y*b.y + b.z*b.z);
        if (bnorm > 0.0f) {
            b.x /= bnorm;
            b.y /= bnorm;
            b.z /= bnorm;
        }

        // Reference gravity in world frame (+Z up → gravity is down)
        constexpr Vector3D_s a = {0.0f, 0.0f, -1.0f};

        // Compute quaternion that rotates a → b
        const Vector3D_s v = {
            a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x
        };

        const float dot = a.x*b.x + a.y*b.y + a.z*b.z;
        Quaternion_s q{v.x, v.y, v.z, 1.0f + dot};

        // Handle the case where vectors are opposite (e.g. device upside-down)
        if (q.w < 1e-6f) {
            // Pick an arbitrary orthogonal axis
            const Vector3D_s ortho = (std::fabs(a.x) < std::fabs(a.z)) ? Vector3D_s{1,0,0} : Vector3D_s{0,1,0};
            const Vector3D_s v_alt = {
                a.y*ortho.z - a.z*ortho.y,
                a.z*ortho.x - a.x*ortho.z,
                a.x*ortho.y - a.y*ortho.x
            };
            q.x = v_alt.x;
            q.y = v_alt.y;
            q.z = v_alt.z;
            q.w = 0.0f;
        }

        normalize(q);
        return q;
    }

    static float angleBetween(const Quaternion_s& q1, const Quaternion_s& q2) {
        // q_rel = conjugate(q1) * q2
        Quaternion_s q_rel = multiply(conjugate(q1), q2);
        normalize(q_rel);

        // Clamp w to avoid numerical issues (arccos domain)
        float w = q_rel.w;
        if (w > 1.0f) w = 1.0f;
        if (w < -1.0f) w = -1.0f;

        // Angle = 2 * arccos(w)
        return 2.0f * std::acos(w);
    }

    // Identity Quaternion2 factory
     static Quaternion_s identity() {
        return {0, 0, 0, 1};
    }
};

class QuaternionTransform final : public Vector3DTransform {
public:
    explicit QuaternionTransform(const Quaternion_s& q) : m_q(q) {}

    Vector3D_s transform(const Vector3D_s& v) const override {
        return QuaternionHelper::rotateVector(m_q, v);;
    }

    QuaternionTransform localRotateXDeg(const float degrees) const {
        const float radians = degrees * static_cast<float>(M_PI) / 180.0f;
        const float half = radians * 0.5f;
        const float s = std::sin(half);
        const float c = std::cos(half);
        Quaternion_s rot{s, 0.0f, 0.0f, c};
        rot = QuaternionHelper::multiply(m_q, rot);
        QuaternionHelper::normalize(rot);
        return QuaternionTransform(rot);
    }

    // Rotate around Y axis
    QuaternionTransform localRotateYDeg(const float degrees) const {
        const float radians = degrees * static_cast<float>(M_PI) / 180.0f;
        const float half = radians * 0.5f;
        const float s = std::sin(half);
        const float c = std::cos(half);
        Quaternion_s rot{0.0f, s, 0.0f, c};
        rot = QuaternionHelper::multiply(m_q, rot);
        QuaternionHelper::normalize(rot);
        return QuaternionTransform(rot);
    }

    // Rotate around Z axis
    QuaternionTransform localRotateZDeg(const float degrees) const {
        const float radians = degrees * static_cast<float>(M_PI) / 180.0f;
        const float half = radians * 0.5f;
        const float s = std::sin(half);
        const float c = std::cos(half);
        Quaternion_s rot{0.0f, 0.0f, s, c};
        rot = QuaternionHelper::multiply(m_q, rot);
        QuaternionHelper::normalize(rot);
        return QuaternionTransform(rot);
    }



private:
    Quaternion_s m_q = QuaternionHelper::identity();
};


#endif // ROTATIONHELO_H
