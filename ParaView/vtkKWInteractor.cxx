/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWToolbar.h"
#include "vtkKWInteractor.h"

int vtkKWInteractorCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWInteractor::vtkKWInteractor()
{
  this->CommandFunction = vtkKWInteractorCommand;
  this->RenderView = NULL;
  this->SelectedState = 0;
  this->ToolbarButton = NULL;
  this->Toolbar = NULL;
}

//----------------------------------------------------------------------------
vtkKWInteractor::~vtkKWInteractor()
{
  this->SetRenderView(NULL);
  this->SetToolbarButton(NULL);
  if (this->Toolbar)
    {
    this->Toolbar->Delete();
    this->Toolbar = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWInteractor::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("widget already created");
    return;
    }
  this->SetApplication(app);
  
  // create the main frame for this widget
  this->Script( "frame %s", this->GetWidgetName());

}


//----------------------------------------------------------------------------
// Lets not "Register" the RenderView.
void vtkKWInteractor::SetRenderView(vtkPVRenderView *view)
{
  this->RenderView = view;
}

//----------------------------------------------------------------------------
void vtkKWInteractor::Select()
{
  if (this->SelectedState)
    {
    return;
    }
  if (this->ToolbarButton)
    {
    this->ToolbarButton->SetState(1);
    }
  if (this->Toolbar)
    {
    this->Script("pack %s -side left -expand no -fill none",
                 this->Toolbar->GetWidgetName());
    }
  this->SelectedState = 1;
}

//----------------------------------------------------------------------------
void vtkKWInteractor::Deselect()
{
  if ( ! this->SelectedState)
    {
    return;
    }
  if (this->ToolbarButton)
    {
    this->ToolbarButton->SetState(0);
    }
  if (this->Toolbar)
    {
    this->Script("pack forget %s", this->Toolbar->GetWidgetName());
    }
  this->SelectedState = 0;
}

