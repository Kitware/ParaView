/*=========================================================================

  Program:   ParaView
  Module:    vtkUndoStackInternal.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkUndoStackInternal_h
#define vtkUndoStackInternal_h

#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkUndoSet.h"      // for vtkUndoSet

#include <string> // for std::string
#include <vector> // for std::vector

class vtkUndoStackInternal
{
public:
  struct Element
  {
    std::string Label;
    vtkSmartPointer<vtkUndoSet> UndoSet;
    Element(const char* label, vtkUndoSet* set)
    {
      this->Label = label;
      this->UndoSet = vtkSmartPointer<vtkUndoSet>::New();
      for (int i = 0, nb = set->GetNumberOfElements(); i < nb; i++)
      {
        this->UndoSet->AddElement(set->GetElement(i));
      }
    }
  };
  typedef std::vector<Element> VectorOfElements;
  VectorOfElements UndoStack;
  VectorOfElements RedoStack;
};

#endif
//****************************************************************************
// VTK-HeaderTest-Exclude: vtkUndoStackInternal.h
