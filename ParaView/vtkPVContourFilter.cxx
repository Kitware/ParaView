/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVContourFilter.cxx
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

#include "vtkPVContourFilter.h"
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"

int vtkPVContourFilterCommand(ClientData cd, Tcl_Interp *interp,
			      int argc, char *argv[]);

vtkPVContourFilter::vtkPVContourFilter()
{
  this->CommandFunction = vtkPVContourFilterCommand;
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  this->ContourValueEntry = vtkKWEntry::New();
  this->ContourValueEntry->SetParent(this);
  this->ContourValueLabel = vtkKWLabel::New();
  this->ContourValueLabel->SetParent(this);
  
  this->Contour = vtkContourFilter::New();
}

vtkPVContourFilter::~vtkPVContourFilter()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ContourValueEntry->Delete();
  this->ContourValueEntry = NULL;
  this->ContourValueLabel->Delete();
  this->ContourValueLabel = NULL;
  
  this->Contour->Delete();
  this->Contour = NULL;
}

vtkPVContourFilter* vtkPVContourFilter::New()
{
  return new vtkPVContourFilter();
}

void vtkPVContourFilter::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("vtkPVContourFilter already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->ContourValueLabel->Create(this->Application, "");
  this->ContourValueLabel->SetLabel("Contour Value:");
  this->ContourValueEntry->Create(this->Application, "");
  this->ContourValueEntry->SetValue(this->GetContour()->GetValue(0), 2);
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ContourValueChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
  this->Script("pack %s %s -side left -anchor w",
	       this->ContourValueLabel->GetWidgetName(),
	       this->ContourValueEntry->GetWidgetName());
}


void vtkPVContourFilter::SetInput(vtkPVData *data)
{
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
  if (data)
    {
    data->Register(this);
    this->Input = data;
    }
}


// We need to look at our input to make our output.
//  I was originally thinking of MakeObject, but this will do for now.
vtkPVData *vtkPVContourFilter::GetDataWidget()
{
  if (this->DataWidget == NULL)
    {
    if (this->Input == NULL)
      {
      vtkErrorMacro("You must set the input before you get the output.");
      return;
      }
    if (this->Input->IsA("vtkPVPolyData"))
      {
      vtkPVPolyData *pd = vtkPVPolyData::New();
      pd->SetPolyData(this->Elevation->GetPolyDataOutput());
      this->SetDataWidget(pd);
      pd->Delete();
      return this->DataWidget;    
      }
    if (this->Input->IsA("vtkPVImage"))
      {
      vtkPVImage *d = vtkPVImage::New();
      d->SetImageData(this->Elevation->GetImageDataOutput());
      this->SetDataWidget(d);
      d->Delete();
      return this->DataWidget;    
      }
    vtkErrorMacro("Have not implemented make object " << this->Input->GetClassName());
  }

  return this->DataWidget;
}

void vtkPVContourFilter::ContourValueChanged()
{  
  this->Contour->SetValue(0, this->ContourValueEntry->GetValueAsFloat());
  this->Contour->Modified();
  this->Contour->Update();
  
  this->Composite->GetView()->Render();
}


