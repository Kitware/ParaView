/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInformationKeys.h"

#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationStringKey.h"

vtkInformationKeyMacro(vtkPVInformationKeys, TIME_LABEL_ANNOTATION, String);
vtkInformationKeyRestrictedMacro(vtkPVInformationKeys, WHOLE_BOUNDING_BOX, DoubleVector, 6);
