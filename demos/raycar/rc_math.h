
#if !defined(rc_math_h)
#define rc_math_h

#include <string.h>
#include <stdexcept>

struct Vec3
{
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3(float const *ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}
    float x, y, z;
};

struct Rgba
{
    Rgba() : r(1), g(1), b(1), a(1) {}
    Rgba(float r_, float g_, float b_, float a_=1) : r(r_), g(g_), b(b_), a(a_) {}
    float r, g, b, a;
};

struct Matrix
{
    //  Using column vectors on the right
    Matrix() { memset(rows, 0, sizeof(rows)); }
    Matrix(float const *idata) { memcpy(rows, idata, sizeof(rows)); }
    static const Matrix identity;
    float rows[4][4];   //  NOT stored in GL order!
    inline void setRow(size_t r, Vec3 const &d)
    {
        if (r > 3)
        {
            throw std::runtime_error("Invalid row in setRow()");
        }
        memcpy(rows[r], &d, sizeof(d));
    }
    inline Vec3 getRow(size_t r) const
    {
        if (r > 3)
        {
            throw std::runtime_error("Invalid row in getRow()");
        }
        return Vec3(rows[r]);
    }
    inline void setColumn(size_t c, Vec3 const &v)
    {
        if (c > 3)
        {
            throw std::runtime_error("Invalid column in setColumn()");
        }
        rows[0][c] = v.x;
        rows[1][c] = v.y;
        rows[2][c] = v.z;
    }
    inline Vec3 getColumn(size_t c) const
    {
        if (c > 3)
        {
            throw std::runtime_error("Invalid column in getColumn()");
        }
        return Vec3(rows[0][c], rows[1][c], rows[2][c]);
    }
    inline void setTranslation(Vec3 const &x)
    {
        rows[0][3] = x.x;
        rows[1][3] = x.y;
        rows[2][3] = x.z;
        rows[3][3] = 1;
    }
    inline Vec3 translation() const
    {
        return Vec3(rows[0][3], rows[1][3], rows[2][3]);
    }
};

void addTo(Vec3 &left, Vec3 const &right);
void subFrom(Vec3 &left, Vec3 const &right);
void scale(Vec3 &io, float l);
void cross(Vec3 &out, Vec3 const &left, Vec3 const &right);
void normalize(Vec3 &n);
void minimize(Vec3 &io, Vec3 const &i);
void maximize(Vec3 &io, Vec3 const &i);

float dot(Vec3 const &a, Vec3 const &b);
float length(Vec3 const &l);
float lengthSquared(Vec3 const &l);
bool equals(Vec3 const &left, Vec3 const &right);

void multiply(Matrix const &left, Matrix const &right, Matrix &result);
void multiply(Matrix const &left, Vec3 &right);

#endif  //  rc_math_h

