/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDummyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDummyWidget.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDummyWidget);
vtkCxxRevisionMacro(vtkPVDummyWidget, "1.6");

//----------------------------------------------------------------------------
vtkPVDummyWidget::vtkPVDummyWidget()
{
}

//----------------------------------------------------------------------------
vtkPVDummyWidget::~vtkPVDummyWidget()
{
}

//----------------------------------------------------------------------------
void vtkPVDummyWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
}

//----------------------------------------------------------------------------
vtkPVDummyWidget* vtkPVDummyWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVDummyWidget::SafeDownCast(clone);
}


//----------------------------------------------------------------------------
void vtkPVDummyWidget::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                                      vtkArrayMap<vtkPVWidget*, 
                                      vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
}

//----------------------------------------------------------------------------
int vtkPVDummyWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVDummyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

