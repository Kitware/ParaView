/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMethodInterface.cxx
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
#include "vtkPVMethodInterface.h"
#include "vtkStringList.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkPVMethodInterface::vtkPVMethodInterface()
{
  this->VariableName = NULL;
  this->SetCommand = NULL;
  this->GetCommand = NULL;
  this->ArgumentTypes = vtkIdList::New();
  this->WidgetType = VTK_PV_METHOD_WIDGET_ENTRY;
  this->SelectionEntries = NULL;
  this->FileExtension = NULL;
  this->BalloonHelp = NULL;
}

//----------------------------------------------------------------------------
vtkPVMethodInterface::~vtkPVMethodInterface()
{
  this->SetVariableName(NULL);
  this->SetSetCommand(NULL);
  this->SetGetCommand(NULL);
  this->ArgumentTypes->Delete();
  this->ArgumentTypes = NULL;
  if (this->SelectionEntries)
    {
    this->SelectionEntries->Delete();
    this->SelectionEntries = NULL;
    }
  this->SetFileExtension(NULL);
  if (this->BalloonHelp)
    {
    delete [] this->BalloonHelp;
    this->BalloonHelp = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVMethodInterface* vtkPVMethodInterface::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVMethodInterface");
  if(ret)
    {
    return (vtkPVMethodInterface*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVMethodInterface;
}

//----------------------------------------------------------------------------
void vtkPVMethodInterface::AddArgumentType(int type)
{
  this->ArgumentTypes->InsertNextId(type);
}

//----------------------------------------------------------------------------
void vtkPVMethodInterface::AddSelectionEntry(int idx, char *string)
{
  if (this->SelectionEntries == NULL)
    {
    this->SelectionEntries = vtkStringList::New();
    }
  this->SelectionEntries->SetString(idx, string);
}

//----------------------------------------------------------------------------
void vtkPVMethodInterface::SetWidgetType(int type)
{
  if (this->WidgetType == type)
    {
    return;
    }
  this->Modified();
  this->WidgetType = type;
  
  if (this->WidgetType == VTK_PV_METHOD_WIDGET_TOGGLE)
    {
    this->ArgumentTypes->Reset();
    this->AddIntegerArgument();
    }
  if (this->WidgetType == VTK_PV_METHOD_WIDGET_SELECTION)
    {
    this->ArgumentTypes->Reset();
    this->AddIntegerArgument();
    }
  if (this->WidgetType == VTK_PV_METHOD_WIDGET_FILE)
    {
    this->ArgumentTypes->Reset();
    this->AddStringArgument();
    }
}

