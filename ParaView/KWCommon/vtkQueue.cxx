/*=========================================================================

  Module:    vtkQueue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkQueue.txx"
#include "vtkVector.txx"

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_EXPORT vtkAbstractList<void*>;
template class VTK_EXPORT vtkAbstractList<vtkObject*>;
template class VTK_EXPORT vtkVector<void*>;
template class VTK_EXPORT vtkVector<vtkObject*>;
template class VTK_EXPORT vtkQueue<void*>;
template class VTK_EXPORT vtkQueue<vtkObject*>;

#endif



