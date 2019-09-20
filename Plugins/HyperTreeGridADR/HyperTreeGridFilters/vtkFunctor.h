/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFunctor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkFunctor_h
#define vtkFunctor_h

#include <cmath>
#include <iostream>

#define vtkFunctorCompMacro                                                                        \
  template <class FunctorT>                                                                        \
  inline bool operator==(const FunctorT&) const                                                    \
  {                                                                                                \
    return false;                                                                                  \
  }                                                                                                \
  template <class FunctorT>                                                                        \
  inline bool operator!=(const FunctorT& f) const                                                  \
  {                                                                                                \
    return !(*this == f);                                                                          \
  }

#define vtkDefaultSpecializationFunctorCompMacro(functor)                                          \
  template <>                                                                                      \
  bool functor::operator==(const functor&) const                                                   \
  {                                                                                                \
    return true;                                                                                   \
  }

/**
 * @class   vtkIdentityFunctor
 * @brief   Functor f(x) = x
 */

struct vtkIdentityFunctor
{
  double operator()(double x) { return x; }
  vtkFunctorCompMacro;
};

/**
 * @class   vtkSquareFunctor
 * @brief   Functor f(x) = x^2
 */
struct vtkSquareFunctor
{
  double operator()(double x) { return x * x; }
  vtkFunctorCompMacro;
};

/**
 * @class   vtkLogFunctor
 * @brief   Functor f(x) = log(x)
 */
struct vtkLogFunctor
{
  double operator()(double x) { return std::log(x); }
  vtkFunctorCompMacro;
};

/**
 * @class   vtkInverseFunctor
 * @brief   Functor f(x) = 1/x
 */
struct vtkInverseFunctor
{
  double operator()(double x) { return 1.0 / x; }
  vtkFunctorCompMacro;
};

/**
 * @class   vtkEntropyFunctor
 * @brief   Functor f(x) = x log(x)
 */
struct vtkEntropyFunctor
{
  double operator()(double x) { return x * std::log(x); }
  vtkFunctorCompMacro;
};

#endif
