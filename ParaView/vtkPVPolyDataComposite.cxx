/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataComposite.cxx
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

#include "vtkPVPolyDataComposite.h"
#include "vtkKWLabel.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkPVWindow.h"

vtkPVPolyDataComposite::vtkPVPolyDataComposite()
{ 
  this->Notebook = vtkKWNotebook::New();
  this->PolyData = NULL;
  this->ConeSource = vtkPVConeSource::New();
  
  this->NotebookCreated = 0;

  this->Label1 = "1";
  this->Label2 = "2";
  
  this->Window = NULL;
}

vtkPVPolyDataComposite* vtkPVPolyDataComposite::New()
{
  return new vtkPVPolyDataComposite();
}

vtkPVPolyDataComposite::~vtkPVPolyDataComposite()
{
  this->Notebook->SetParent(NULL);
  this->Notebook->Delete();
  this->Notebook = NULL;
  
  this->ConeSource->Delete();
  this->ConeSource = NULL;
}


void vtkPVPolyDataComposite::Select(vtkKWView *view)
{
}

void vtkPVPolyDataComposite::Deselect(vtkKWView *view)
{
}

void vtkPVPolyDataComposite::SetWindow(vtkPVWindow *window)
{
  if (this->Window == NULL)
    {
    this->Window = vtkPVWindow::New();
    }
  
  this->Window = window;
}

vtkPVWindow* vtkPVPolyDataComposite::GetWindow()
{
  return this->Window;
}

vtkProp* vtkPVPolyDataComposite::GetProp()
{
  return this->PolyData->GetProp();
}

vtkPVPolyDataComposite* vtkPVPolyDataComposite::GetComposite()
{
  return this;
}

void vtkPVPolyDataComposite::CreateProperties(vtkKWApplication *app, char *args)
{
  this->SetApplication(app);
  
  if (this->NotebookCreated)
    {
    return;
    }
  this->NotebookCreated = 1;

  vtkKWWidget *menubutton = vtkKWWidget::New();
  
  this->Notebook->Create(app, args);
  this->Notebook->AddPage(this->Label1);
  this->Notebook->AddPage(this->Label2);
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->Notebook->Raise(this->Label1);
  
  if (this->PolyData == NULL)
    {
    this->PolyData = vtkPVPolyData::New();
    this->ConeSource->SetConeSource();
    this->PolyData->SetPolyData(this->ConeSource->GetOutput());
    this->PolyData->SetComposite(this);
    }
    
  this->PolyData->SetParent(this->Notebook->GetFrame(this->Label1));
  this->PolyData->Create(app, "");
  this->Script("pack %s", this->PolyData->GetWidgetName());
  
  this->ConeSource->SetParent(this->Notebook->GetFrame(this->Label2));
  this->ConeSource->Create(app, "");
  this->Script("pack %s", this->ConeSource->GetWidgetName());
}

void vtkPVPolyDataComposite::SetPropertiesParent(vtkKWWidget *parent)
{
  this->Notebook->SetParent(parent);
}

vtkKWWidget *vtkPVPolyDataComposite::GetPropertiesParent()
{
  return this->Notebook->GetParent();
}

vtkKWWidget *vtkPVPolyDataComposite::GetProperties()
{
  return this->Notebook;
}

void vtkPVPolyDataComposite::SetTabLabels(char *label1, char *label2)
{
  this->Label1 = label1;
  this->Label2 = label2;
}

vtkPVConeSource* vtkPVPolyDataComposite::GetConeSource()
{
  return this->ConeSource;
}

void vtkPVPolyDataComposite::SetPVPolyData(vtkPVPolyData *poly)
{
  this->PolyData = poly;
}
