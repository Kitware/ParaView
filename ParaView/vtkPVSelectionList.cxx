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

  this->Value = 0;
  this->Label = NULL;
}

//----------------------------------------------------------------------------
vtkPVSelectionList::~vtkPVSelectionList()
{
}

//----------------------------------------------------------------------------
vtkPVSelectionList* vtkPVSelectionList::New()
{
  return new vtkPVSelectionList();
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::Create(vtkKWApplication *app)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Object has not been cloned yet.");
    return 0;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), "");

  //this->Script("pack %s", this->ActorCompositeButton->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::AddItem(char *name, int value)
{
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetValue(int value)
{
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::GetValue()
{
  return this->Value;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetLabel(char *label)
{
}

