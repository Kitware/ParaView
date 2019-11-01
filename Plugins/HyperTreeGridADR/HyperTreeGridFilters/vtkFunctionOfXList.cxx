#include "vtkFunctionOfXList.h"

#include "vtkSetGet.h"

#include <cmath>

double VTK_FUNC_X(double x)
{
  return x;
}

double VTK_FUNC_X2(double x)
{
  return x * x;
}

double VTK_FUNC_NXLOGX(double x)
{
  return -x * std::log(x);
}

double VTK_FUNC_1_X(double x)
{
  return 1.0 / x;
}

double VTK_FUNC_NULL(double vtkNotUsed(x))
{
  return 0.0;
}

double VTK_FUNC_1(double vtkNotUsed(x))
{
  return 1.0;
}
