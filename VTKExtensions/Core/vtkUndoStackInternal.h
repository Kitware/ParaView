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
#include "vtkSmartPointer.h"
#include "vtkUndoSet.h"

#include <string>
#include <vector>

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
//****************************************************************************
// VTK-HeaderTest-Exclude: vtkUndoStackInternal.h
