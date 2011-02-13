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

#include <vtkstd/string>
#include <vtkstd/vector>

class vtkUndoStackInternal
{
public:
  struct Element
    {
    vtkstd::string Label;
    vtkSmartPointer<vtkUndoSet> UndoSet;
    Element(const char* label, vtkUndoSet* set)
      {
      this->Label = label;
      this->UndoSet = vtkSmartPointer<vtkUndoSet>::New();
      for(int i=0,nb=set->GetNumberOfElements(); i < nb; i++)
        {
        this->UndoSet->AddElement(set->GetElement(i));
        }
      }
    };
  typedef vtkstd::vector<Element> VectorOfElements;
  VectorOfElements UndoStack;
  VectorOfElements RedoStack;
};
//****************************************************************************

