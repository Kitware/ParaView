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

vtkPVComposite::vtkPVComposite()
{
  this->Notebook = vtkKWNotebook::New();
  this->Image = NULL;
  this->ImageReader = vtkPVImageReader::New();
  
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
  
  this->Image->Delete();
  this->Image = NULL;
  
  this->ImageReader->Delete();
  this->ImageReader = NULL;
}


void vtkPVComposite::Select(vtkKWView *view)
{
}

void vtkPVComposite::Deselect(vtkKWView *view)
{
}

vtkProp* vtkPVComposite::GetProp()
{
  return this->Image->GetProp();
}

void vtkPVComposite::CreateProperties(vtkKWApplication *app, char *args)
{ 
  this->SetApplication(app);
  
  if (this->NotebookCreated)
    {
    return;
    }
  this->NotebookCreated = 1;
  
  this->Notebook->Create(app, args);
  this->Notebook->AddPage("Image");
  this->Notebook->AddPage("Image Reader");
  
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
  this->Notebook->Raise("Image");
  
  if (this->Image == NULL)
    {
    this->Image = vtkPVImage::New();
    }
  
  this->Image->SetParent(this->Notebook->GetFrame("Image"));
  this->Image->Create(app, "");
  this->Script("pack %s", this->Image->GetWidgetName());

  this->ImageReader->SetParent(this->Notebook->GetFrame("Image Reader"));
  this->ImageReader->Create(app, "");
  this->Script("pack %s", this->ImageReader->GetWidgetName());
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
