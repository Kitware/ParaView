/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVComposite.cxx
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

#include "vtkPVComposite.h"
#include "vtkKWLabel.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkPVWindow.h"

vtkPVComposite::vtkPVComposite()
{
  this->Notebook = vtkKWNotebook::New();
  this->Data = NULL;
  this->Source = NULL;
  this->Window = NULL;
  
  this->NotebookCreated = 0;
}

vtkPVComposite* vtkPVComposite::New()
{
  return new vtkPVComposite();
}

vtkPVComposite::~vtkPVComposite()
{
  this->Notebook->SetParent(NULL);
  this->Notebook->Delete();
  this->Notebook = NULL;
  
  this->SetData(NULL);
  this->SetSource(NULL);
}

void vtkPVComposite::SetWindow(vtkPVWindow *window)
{
  if (this->Window == window)
    {
    return;
    }
  this->Modified();

  if (this->Window)
    {
    vtkPVWindow *tmp = this->Window;
    this->Window = NULL;
    //Register and UnRegister are commented out because including
    //these lines causes ParaView to crash when you try to exit.
    //We think it is probably some weirdness with reference counting.
//    tmp->UnRegister(this);
    }
  if (window)
    {
    this->Window = window;
//    window->Register(this);
    }
}

vtkPVWindow *vtkPVComposite::GetWindow()
{
  return this->Window;
}

void vtkPVComposite::Select(vtkKWView *view)
{
}

void vtkPVComposite::Deselect(vtkKWView *view)
{
}

vtkProp* vtkPVComposite::GetProp()
{
  return this->Data->GetProp();
}

void vtkPVComposite::CreateProperties(vtkKWApplication *app, char *args)
{ 
  const char *dataPage, *sourcePage;

  if (this->Data == NULL || this->Source == NULL)
    {
    vtkErrorMacro("You need to set the data and source before you create a composite");
    return;
    }

  this->SetApplication(app);
  
  if (this->NotebookCreated)
    {
    return;
    }
  this->NotebookCreated = 1;
  
  this->Notebook->Create(app, args);
  sourcePage = this->Source->GetClassName();
  this->Notebook->AddPage(sourcePage);
  dataPage = this->Data->GetClassName();
  this->Notebook->AddPage(dataPage);
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
    
  this->Data->SetParent(this->Notebook->GetFrame(dataPage));
  this->Data->Create(app, "");
  this->Script("pack %s", this->Data->GetWidgetName());

  this->Source->SetParent(this->Notebook->GetFrame(sourcePage));
  this->Source->Create(app, "");
  this->Script("pack %s", this->Source->GetWidgetName());
}

void vtkPVComposite::SetPropertiesParent(vtkKWWidget *parent)
{
  this->Notebook->SetParent(parent);
}

vtkKWWidget *vtkPVComposite::GetPropertiesParent()
{
  return this->Notebook->GetParent();
}

vtkKWWidget *vtkPVComposite::GetProperties()
{
  return this->Notebook;
}

void vtkPVComposite::SetSource(vtkPVSource *source)
{
  if (this->Source == source)
    {
    return;
    }
  this->Modified();

  if (this->Source)
    {
    vtkPVSource *tmp = this->Source;
    this->Source = NULL;
    tmp->SetComposite(NULL);
    tmp->UnRegister(this);
    }
  if (source)
    {
    this->Source = source;
    source->Register(this);
    source->SetComposite(this);
    }
}

void vtkPVComposite::SetData(vtkPVData *data)
{
  if (this->Data == data)
    {
    return;
    }
  this->Modified();

  if (this->Data)
    {
    vtkPVData *tmp = this->Data;
    this->Data = NULL;
    tmp->SetComposite(NULL);
    tmp->UnRegister(this);
    }
  if (data)
    {
    this->Data = data;
    data->Register(this);
    data->SetComposite(this);
    }
}
