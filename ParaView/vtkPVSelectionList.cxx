/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSelectionList.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkPVSelectionList.h"

int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVSelectionList::vtkPVSelectionList()
{
  this->CommandFunction = vtkPVSelectionListCommand;

  this->CurrentValue = 0;
  this->CurrentName = NULL;
  
  this->MenuButton = vtkPVMenuButton::New();
  this->Label = vtkKWLabel::New();
}

//----------------------------------------------------------------------------
vtkPVSelectionList::~vtkPVSelectionList()
{
  this->SetCurrentName(NULL);
  
  this->MenuButton->Delete();
  this->MenuButton = NULL;
  this->Label->Delete();
  this->Label = NULL;
}

//----------------------------------------------------------------------------
vtkPVSelectionList* vtkPVSelectionList::New()
{
  return new vtkPVSelectionList();
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return 0;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->MenuButton->SetParent(this);
  this->MenuButton->Create(app, "");
  this->Script("pack %s -side left", this->MenuButton->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::AddItem(const char *name, int value)
{
  char tmp[1024];
  
  sprintf(tmp, "SelectCallback %s %d", name, value);
  this->MenuButton->AddCommand(name, this, tmp);
  
  if (value == this->CurrentValue)
    {
    this->MenuButton->SetButtonText(name);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SelectCallback(const char *name, int value)
{
  this->SetCurrentValue(value);
  this->SetCurrentName(name);
  
  this->MenuButton->SetButtonText(name);
}

