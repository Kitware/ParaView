/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVData.cxx
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
#include "vtkPVData.h"
#include "vtkPVPolyData.h" //because contour produces poly data as output
#include "vtkPVComposite.h"
#include "vtkPVSource.h"
#include "vtkKWView.h"

#include "vtkPVWindow.h"
#include "vtkKWApplication.h"
#include "vtkPVContourFilter.h"


int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


vtkPVData::vtkPVData()
{
  this->CommandFunction = vtkPVDataCommand;

  this->Data = NULL;
  this->SourceWidget = NULL;
  
  this->FiltersMenuButton = vtkPVMenuButton::New();
  
  this->Mapper = vtkDataSetMapper::New();
  this->Actor = vtkActor::New();
  
}

vtkPVData::~vtkPVData()
{
  this->SetData(NULL);
  this->SetSourceWidget(NULL);

  this->FiltersMenuButton->Delete();
  this->FiltersMenuButton = NULL;
  
  this->Mapper->Delete();
  this->Mapper = NULL;
  
  this->Actor->Delete();
  this->Actor = NULL;
}

vtkPVData* vtkPVData::New()
{
  return new vtkPVData();
}

void vtkPVData::Contour()
{
  vtkPVContourFilter *contour;
  vtkPVPolyData *pd;
  vtkPVComposite *newComp;
  float *range;
  
  contour = vtkPVContourFilter::New();
  contour->GetContour()->SetInput(this->GetData());
  
  range = this->GetData()->GetScalarRange();
  contour->GetContour()->SetValue(0, (range[1]-range[0])/2.0);
  
  pd = vtkPVPolyData::New();
  pd->SetPolyData(contour->GetContour()->GetOutput());
    
  newComp = vtkPVComposite::New();
  newComp->SetSource(contour);
  newComp->SetData(pd);

  vtkPVWindow *window = this->GetComposite()->GetWindow();
  newComp->SetPropertiesParent(window->GetDataPropertiesParent());
  newComp->CreateProperties(this->Application, "");
  this->GetComposite()->GetView()->AddComposite(newComp);
  this->GetComposite()->GetProp()->VisibilityOff();
  
  newComp->SetWindow(window);
  
  window->SetCurrentDataComposite(newComp);
  
  pd->GetComposite()->GetView()->Render();
  
  pd->Delete();
  newComp->Delete();
}

void vtkPVData::AddCommonWidgets(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Label already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);

  this->GetData()->Update();
  
  this->FiltersMenuButton->SetParent(this);
  this->FiltersMenuButton->Create(app, "");
  this->FiltersMenuButton->SetButtonText("Filters");
  this->FiltersMenuButton->AddCommand("vtkContourFilter", this,
				      "Contour");
  if (this->GetData()->GetPointData()->GetScalars() == NULL)
    {
    this->Script("%s entryconfigure 3 -state disabled",
		 this->FiltersMenuButton->GetMenu()->GetWidgetName());
    }
  else
    {
    this->Script("%s entryconfigure 3 -state normal",
		 this->FiltersMenuButton->GetMenu()->GetWidgetName());
    }
  
  this->Script("pack %s", this->FiltersMenuButton->GetWidgetName());
}

vtkProp* vtkPVData::GetProp()
{
  return this->Actor;
}

// MAYBE WE SHOULD NOT REFERENCE COUNT HERE BECAUSE NO ONE BUT THE 
// SOURCE WIDGET WILL REFERENCE THE DATA WIDGET.
void vtkPVData::SetSourceWidget(vtkPVSource *source)
{
  if (this->SourceWidget == source)
    {
    return;
    }
  this->Modified();

  if (this->SourceWidget)
    {
    vtkPVSource *tmp = this->SourceWidget;
    this->SourceWidget = NULL;
    tmp->UnRegister(this);
    }
  if (source)
    {
    this->SourceWidget = source;
    source->Register(this);
    }
}


vtkPVComposite *vtkPVData::GetComposite()
{
  if (this->SourceWidget == NULL)
    {
    return NULL;
    }

  return this->SourceWidget->GetComposite();
}