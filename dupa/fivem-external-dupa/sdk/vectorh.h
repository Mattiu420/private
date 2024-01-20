#ifndef VECTOR_H
#define VECTOR_H

#include <cmath>
#include <immintrin.h>
#include "../overlay/overlay.h"


struct Vec2
{
    float x, y, z;
};

struct Vec3
{
    float x, y, z;

    Vec3 operator-(const Vec3& other) const {
        return { x - other.x, y - other.y, z - other.z };
    }

    Vec3 operator+(const Vec3& other) const {
        return { x + other.x, y + other.y, z + other.z };
    }

    Vec3 operator/(float scalar) const {
        return { x / scalar, y / scalar, z / scalar };
    }

    Vec3 operator*(float scalar) const {
        return { x * scalar, y * scalar, z * scalar };
    }

    float Length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    void Normalize() {
        float len = Length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    void Clamp() {
        if (y > 89.0f) y = 89.0f;
        else if (y < -89.0f) y = -89.0f;

        if (x > 180.0f) x = 180.0f;
        else if (x < -180.0f) x = -180.0f;

        z = 0;
    }

    Vec3 operator/(float i) {
        return {
            x / i,
            y / i,
            z / i
        };
    }

    Vec3 operator*(float i) {
        return {
            x * i,
            y * i,
            z * i
        };
    }

    static Vec3 FromM128(__m128 in)
    {
        return Vec3
        {
            in.m128_f32[0],
            in.m128_f32[1],
            in.m128_f32[2]
        };
    }

    static Vec3 FromD3DXVECTOR3(const D3DXVECTOR3& v)
    {
        return Vec3{ v.x, v.y, v.z };
    }

    Vec3 CalcAngle(const Vec3& targetPos) const {
        Vec3 direction = targetPos - *this;
        float pitch = asinf(direction.y / direction.Length());
        float yaw = atan2f(direction.x, direction.z);
        pitch = pitch * (180.0f / 3.1415926535f);
        yaw = yaw * (180.0f / 3.1415926535f);
        return { pitch, yaw, 0.0f };
    }
};

#endif // VECTOR_H

struct Vec4
{
    float x, y, z, w;
};

float pythag(ImVec2 src, Vec2 dst)
{
    return sqrtf(pow(src.x - dst.x, 2) + pow(src.y - dst.y, 2));
}

float pythagVec3(Vec3 src, Vec3 dst)
{
    return sqrtf(pow(src.x - dst.x, 2) + pow(src.y - dst.y, 2) + pow(src.z - dst.z, 2));
}

int roll(int min, int max)
{
    double x = rand() / static_cast<double>(RAND_MAX + 1);
    return min + static_cast<int>(x * (max - min));
}