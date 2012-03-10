
#include "rc_math.h"
#include <stdexcept>



float idInit[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};
Matrix const Matrix::identity(idInit);

void addTo(Vec3 &left, Vec3 const &right)
{
    left.x += right.x;
    left.y += right.y;
    left.z += right.z;
}

void subFrom(Vec3 &left, Vec3 const &right)
{
    left.x -= right.x;
    left.y -= right.y;
    left.z -= right.z;
}

void scale(Vec3 &io, float l)
{
    io.x *= l;
    io.y *= l;
    io.z *= l;
}

void cross(Vec3 &out, Vec3 const &left, Vec3 const &right)
{
    //  be careful not to write to a possibly aliased location
    Vec3 r;
    r.x = left.y * right.z - left.z * right.y;
    r.y = left.z * right.x - left.x * right.z;
    r.z = left.x * right.y - left.y * right.x;
    out = r;
}

//  normalize() is careful to be numerically sound
void normalize(Vec3 &n)
{
    float ax = fabs(n.x);
    float ay = fabs(n.y);
    float az = fabs(n.z);
    float scale = 0;
    if (ax > ay)
        if (ax > az)
            scale = ax;
        else
            scale = az;
    else
        if (ay > az)
            scale = ay;
        else
            scale = az;
    if (scale <= 0)
    {
        throw std::runtime_error("Cannot normalize zero-length vector!");
    }
    //  re-scale so it's "close" to 1 in length -- this avoids precision problems with sqrt
    scale = 1.0f / scale;
    n.x *= scale;
    n.y *= scale;
    n.z *= scale;
    //  then actually normalize
    float l = length(n);
    l = 1.0f / l;
    n.x *= l;
    n.y *= l;
    n.z *= l;
}

float dot(Vec3 const &a, Vec3 const &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length(Vec3 const &l)
{
    return sqrtf(dot(l, l));
}

float lengthSquared(Vec3 const &l)
{
    return dot(l, l);
}

void multiply(Matrix const &left, Matrix &right)
{
    //  be careful to not write to a possibly aliased location
    Matrix out;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            float v = 0;
            for (int i = 0; i < 4; ++i)
            {
                v = v + left.rows[r][i] * right.rows[i][c];
            }
            out.rows[r][c] = v;
        }
    }
    right = out;
}

void multiply(Matrix const &left, Vec3 &right)
{
    //  be careful to not write to a possibly aliased location
    float x = right.x * left.rows[0][0] + right.y * left.rows[0][1] + right.z * left.rows[0][2] + left.rows[0][3];
    float y = right.x * left.rows[1][0] + right.y * left.rows[1][1] + right.z * left.rows[1][2] + left.rows[1][3];
    float z = right.x * left.rows[2][0] + right.y * left.rows[2][1] + right.z * left.rows[2][2] + left.rows[2][3];
    right.x = x;
    right.y = y;
    right.z = z;
}

