#pragma once
#include <cmath>

struct Vec3 {
    float x,y,z;
    Vec3() : x(0),y(0),z(0) {}
    Vec3(float X,float Y,float Z):x(X),y(Y),z(Z){}

    Vec3 operator+(const Vec3& o) const { return {x+o.x,y+o.y,z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x,y-o.y,z-o.z}; }
    Vec3 operator*(float s) const { return {x*s,y*s,z*s}; }

    float Dot(const Vec3& o) const { return x*o.x+y*o.y+z*o.z; }
    float Len() const { return std::sqrt(Dot(*this)); }
    Vec3 Norm() const { float l=Len(); return l>0?(*this)*(1.0f/l):Vec3(); }
};