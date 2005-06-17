/*=========================================================================

  Module:    vtkKWCompositeWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWCompositeWidget.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCompositeWidget );
vtkCxxRevisionMacro(vtkKWCompositeWidget, "1.2");

//----------------------------------------------------------------------------
void vtkKWCompositeWidget::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  // We are cheating a little bit here, but I don't want to subclass
  // from a vtkKWFrame, because it would make vtkKWCompositeWidget
  // a subclass of vtkKWCoreWidget, which makes no sense. Also
  // I don't want users to assume too much about the API of this
  // "frame", it just supposed to be a safe placeholder for UI 
  // sub-elements, something that is "packable".
  
  this->Script("frame %s", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWCompositeWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

