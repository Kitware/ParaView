/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyData.cxx
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
#include "vtkPVPolyData.h"
#include "vtkPVShrinkPolyData.h"
#include "vtkPVElevationFilter.h"
#include "vtkPVConeSource.h"
#include "vtkPVGlyph3D.h"
#include "vtkKWView.h"

#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkPVWindow.h"
#include "vtkKWApplication.h"

#include "vtkConeSource.h"

int vtkPVPolyDataCommand(ClientData cd, Tcl_Interp *interp,
		                     int argc, char *argv[]);


vtkPVPolyData::vtkPVPolyData()
{
  this->CommandFunction = vtkPVPolyDataCommand;
}

vtkPVPolyData::~vtkPVPolyData()
{
}

vtkPVPolyData* vtkPVPolyData::New()
{
  return new vtkPVPolyData();
}

void vtkPVPolyData::Shrink()
{
  //shrink factor defaults to 0.5, which seems reasonable
  vtkPVShrinkPolyData *shrink;
  vtkPVComposite *newComp;
  vtkPVWindow *window = this->GetComposite()->GetWindow();
  
  shrink = vtkPVShrinkPolyData::New();
  shrink->GetShrink()->SetInput(this->GetPolyData());  
    
  newComp = vtkPVComposite::New();
  newComp->SetSource(shrink);
  newComp->SetCompositeName("shrink");
  
  newComp->SetPropertiesParent(window->GetDataPropertiesParent());
  newComp->CreateProperties(this->Application, "");

  this->GetComposite()->GetView()->AddComposite(newComp);

  // Turn this data object off so the next will will not be ocluded.
  this->GetComposite()->GetProp()->VisibilityOff();

  // The window here should probably be replaced with the view.
  newComp->SetWindow(window);
  window->SetCurrentDataComposite(newComp);
  window->GetDataList()->Update();
  
  this->GetComposite()->GetView()->Render();
  
  newComp->Delete();
  shrink->New();
}

void vtkPVPolyData::Glyph()
{
  vtkPVComposite *glyphComp;
  vtkPVConeSource *glyphCone = vtkPVConeSource::New();
  vtkPVGlyph3D *glyph;
  vtkPVComposite *newComp;
  vtkPVWindow *window = this->GetComposite()->GetWindow();

  glyphComp = vtkPVComposite::New();
  glyphComp->SetSource(glyphCone);
  glyphComp->SetPropertiesParent(window->GetDataPropertiesParent());
  glyphComp->CreateProperties(this->Application, "");
  window->GetMainView()->AddComposite(glyphComp);
  glyphComp->SetWindow(window);
  glyphComp->SetCompositeName("glyph comp");
  
  glyph = vtkPVGlyph3D::New();
  glyph->GetGlyph()->SetInput(this->GetPolyData());
  glyph->GetGlyph()->SetSource(glyphCone->GetConeSource()->GetOutput());
  glyph->SetGlyphComposite(glyphComp);
  glyph->GetGlyph()->SetScaleModeToDataScalingOff();
  
  glyphComp->GetProp()->VisibilityOff();
    
  newComp = vtkPVComposite::New();
  newComp->SetSource(glyph);
  newComp->SetCompositeName("glyph");
 
  newComp->SetPropertiesParent(window->GetDataPropertiesParent());
  newComp->CreateProperties(this->Application, "");
  this->GetComposite()->GetView()->AddComposite(newComp);
  this->GetComposite()->GetProp()->VisibilityOff();
  
  newComp->SetWindow(window);
  window->SetCurrentDataComposite(newComp);
  window->GetDataList()->Update();
  
  this->GetComposite()->GetView()->Render();
  
  newComp->Delete();

  glyphCone->Delete();
  glyphComp->Delete();
}

void vtkPVPolyData::Elevation()
{
  vtkPVElevationFilter *elevation;
  vtkPVComposite *newComp;
  float *bounds;
  float low[3], high[3];
  
  bounds = this->GetPolyData()->GetBounds();
  low[0] = 0;
  low[1] = 0;
  low[2] = bounds[4];
  high[0] = 0;
  high[1] = 0;
  high[2] = bounds[5];
  
  elevation = vtkPVElevationFilter::New();
  elevation->GetElevation()->SetInput(this->GetPolyData());
  elevation->GetElevation()->SetLowPoint(low);
  elevation->GetElevation()->SetHighPoint(high);
    
  newComp = vtkPVComposite::New();
  newComp->SetSource(elevation);
  newComp->SetCompositeName("elevation");
  
  vtkPVWindow *window = this->GetComposite()->GetWindow();
  newComp->SetPropertiesParent(window->GetDataPropertiesParent());
  newComp->CreateProperties(this->Application, "");
  this->GetComposite()->GetView()->AddComposite(newComp);
  this->GetComposite()->GetProp()->VisibilityOff();
  
  newComp->SetWindow(window);
  
  window->SetCurrentDataComposite(newComp);
  window->GetDataList()->Update();

  this->GetComposite()->GetView()->Render();
  newComp->Delete();
}

void vtkPVPolyData::Create(vtkKWApplication *app, char *args)
{
  vtkPVData::AddCommonWidgets(app, args);
  
  this->SetApplication(app);
  
  this->FiltersMenuButton->AddCommand("vtkShrinkPolyData", this,
				      "Shrink");
  this->FiltersMenuButton->AddCommand("vtkElevationFilter", this,
				      "Elevation");
  this->FiltersMenuButton->AddCommand("vtkGlyph3D", this,
				      "Glyph");
}

void vtkPVPolyData::SetPolyData(vtkPolyData *data)
{
  this->SetData(data);
  this->Mapper->SetInput(data);
  this->Actor->SetMapper(this->Mapper);
}

vtkPolyData *vtkPVPolyData::GetPolyData()
{
  return (vtkPolyData*)this->Data;
}
