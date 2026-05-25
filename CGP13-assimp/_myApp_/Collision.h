#pragma once

#include <vmath.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

// 바닥, 건물, 장벽, 공격박스
struct AABB
{
    vmath::vec3 min;
    vmath::vec3 max;
};

// 거인, 조사병단
struct Circle
{
    vmath::vec3 position; // 바닥 중심점 기준
    float radius;
    float height;
};

// 입체기동 구현 시 사용
struct Ray
{
    vmath::vec3 origin;
    vmath::vec3 direction;
};
struct RayHit
{
    bool hit = false;
    float distance = FLT_MAX;
    vmath::vec3 point = vmath::vec3(0.0f, 0.0f, 0.0f);
};


inline AABB MakeAABB(const vmath::vec3& center, const vmath::vec3& halfSize)
{
    AABB box;
    box.min = center - halfSize;
    box.max = center + halfSize;
    return box;
}

inline Circle MakeCircle(const vmath::vec3& position, float radius, float height)
{
    Circle circle;
    circle.position = position;
    circle.radius = radius;
    circle.height = height;
    return circle;
}

// AABB와 AABB 충돌 판정
inline bool CheckAABBAABB(const AABB& a, const AABB& b)
{
    return
        a.min[0] <= b.max[0] && a.max[0] >= b.min[0] &&
        a.min[1] <= b.max[1] && a.max[1] >= b.min[1] &&
        a.min[2] <= b.max[2] && a.max[2] >= b.min[2];
}

// XZ 평면에서 AABB와 Circle 충돌 검사
inline bool CheckAABBCircle(const AABB& box, const Circle& circle)
{
    float circleMinY = circle.position[1];
    float circleMaxY = circle.position[1] + circle.height;

    bool yOverlap = circleMinY <= box.max[1] && circleMaxY >= box.min[1];

    if (!yOverlap)
        return false;

    float closestX = std::max(box.min[0], std::min(circle.position[0], box.max[0]));
    float closestZ = std::max(box.min[2], std::min(circle.position[2], box.max[2]));

    float dx = circle.position[0] - closestX;
    float dz = circle.position[2] - closestZ;

    return dx * dx + dz * dz <= circle.radius * circle.radius;
}
inline bool CheckCircleAABB(const Circle& circle, const AABB& box)
{
    return CheckAABBCircle(box, circle);
}

// Circle와 Circle의 충돌 검사
inline bool CheckCircleCircle(const Circle& a, const Circle& b)
{
    float aMinY = a.position[1];
    float aMaxY = a.position[1] + a.height;

    float bMinY = b.position[1];
    float bMaxY = b.position[1] + b.height;

    bool yOverlap =
        aMinY <= bMaxY &&
        aMaxY >= bMinY;

    if (!yOverlap)
        return false;

    float dx = a.position[0] - b.position[0];
    float dz = a.position[2] - b.position[2];

    float radiusSum = a.radius + b.radius;

    return dx * dx + dz * dz <= radiusSum * radiusSum;
}

// 입체기동 구현을 위한 충돌 판정 ---------------------------
inline bool CheckRayAABB(const Ray& ray, const AABB& box, RayHit& outHit)
{
    float tMin = 0.0f;
    float tMax = FLT_MAX;

    for (int i = 0; i < 3; ++i)
    {
        float origin = ray.origin[i];
        float dir = ray.direction[i];

        if (std::fabs(dir) < 0.00001f)
        {
            if (origin < box.min[i] || origin > box.max[i])
                return false;
        }
        else
        {
            float invD = 1.0f / dir;
            float t1 = (box.min[i] - origin) * invD;
            float t2 = (box.max[i] - origin) * invD;

            if (t1 > t2)
                std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax)
                return false;
        }
    }

    outHit.hit = true;
    outHit.distance = tMin;
    outHit.point = ray.origin + ray.direction * tMin;

    return true;
}

inline bool CheckRayCircle(const Ray& ray, const Circle& circle, RayHit& outHit)
{
    bool found = false;
    float bestT = FLT_MAX;

    float minY = circle.position[1];
    float maxY = circle.position[1] + circle.height;

    auto TryUpdateHit = [&](float t)
        {
            if (t < 0.0f)
                return;

            if (t >= bestT)
                return;

            vmath::vec3 p = ray.origin + ray.direction * t;

            float dx = p[0] - circle.position[0];
            float dz = p[2] - circle.position[2];

            bool insideRadius =
                dx * dx + dz * dz <= circle.radius * circle.radius;

            bool insideHeight =
                p[1] >= minY && p[1] <= maxY;

            if (insideRadius && insideHeight)
            {
                bestT = t;
                outHit.hit = true;
                outHit.distance = t;
                outHit.point = p;
                found = true;
            }
        };

    // 1. 원기둥 옆면 검사: XZ 평면에서 ray와 원의 교차 검사
    float ox = ray.origin[0] - circle.position[0];
    float oz = ray.origin[2] - circle.position[2];

    float dx = ray.direction[0];
    float dz = ray.direction[2];

    float a = dx * dx + dz * dz;
    float b = 2.0f * (ox * dx + oz * dz);
    float c = ox * ox + oz * oz - circle.radius * circle.radius;

    if (std::fabs(a) > 0.00001f)
    {
        float discriminant = b * b - 4.0f * a * c;

        if (discriminant >= 0.0f)
        {
            float sqrtD = std::sqrt(discriminant);

            float t1 = (-b - sqrtD) / (2.0f * a);
            float t2 = (-b + sqrtD) / (2.0f * a);

            TryUpdateHit(t1);
            TryUpdateHit(t2);
        }
    }

    // 2. 위/아래 뚜껑 검사
    if (std::fabs(ray.direction[1]) > 0.00001f)
    {
        float tBottom = (minY - ray.origin[1]) / ray.direction[1];
        float tTop = (maxY - ray.origin[1]) / ray.direction[1];

        TryUpdateHit(tBottom);
        TryUpdateHit(tTop);
    }

    return found;
}