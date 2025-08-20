#ifndef __VECTOR_H__
#define __VECTOR_H__

template <typename T> class Vector {
public:
  T x;
  T y;

public:
  Vector() {
    x = 0;
    y = 0;
  }

  Vector(T X, T Y) : x(X), y(Y) {}
};

#endif // !__VECTOR_H__
#define __VECTOR_H__
