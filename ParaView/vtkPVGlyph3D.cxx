/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGlyph3D.cxx
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

#include "vtkPVGlyph3D.h"
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVPolyData.h"

int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
			int argc, char *argv[]);

vtkPVGlyph3D::vtkPVGlyph3D()
{
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  this->GlyphCompositeButton = vtkKWWidget::New();
  this->GlyphCompositeButton->SetParent(this);
  this->ScaleFactorEntry = vtkKWEntry::New();
  this->ScaleFactorEntry->SetParent(this);
  this->ScaleFactorLabel = vtkKWLabel::New();
  this->ScaleFactorLabel->SetParent(this);
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  
  this->Glyph = vtkGlyph3D::New();
}

vtkPVGlyph3D::~vtkPVGlyph3D()
{
  this->GlyphCompositeButton->Delete();
  this->GlyphCompositeButton = NULL;
  this->ScaleFactorEntry->Delete();
  this->ScaleFactorEntry = NULL;
  this->ScaleFactorLabel->Delete();
  this->ScaleFactorLabel = NULL;
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->Glyph->Delete();
  this->Glyph = NULL;
}

vtkPVGlyph3D* vtkPVGlyph3D::New()
{
  return new vtkPVGlyph3D();
}

void vtkPVGlyph3D::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("vtkPVGlyph3D already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->GlyphCompositeButton->Create(this->Application, "button",
			 "-text GetGlyphComposite");
  this->GlyphCompositeButton->SetCommand(this, "ShowGlyphComposite");
  
  this->ScaleFactorEntry->Create(this->Application, "");
  this->ScaleFactorEntry->SetValue(1, 2);
  this->ScaleFactorLabel->Create(this->Application, "");
  this->ScaleFactorLabel->SetLabel("Scale Factor:");
  this->Accept->Create(this->Application, "button",
		       "-text Accept");
  this->Accept->SetCommand(this, "ScaleFactorChanged");
  
  this->Script("pack %s %s %s %s",
	       this->GlyphCompositeButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->ScaleFactorLabel->GetWidgetName(),
	       this->ScaleFactorEntry->GetWidgetName());
}

void vtkPVGlyph3D::SetGlyphComposite(vtkPVComposite *comp)
{
  this->GlyphComposite = comp;
}

void vtkPVGlyph3D::ShowGlyphComposite()
{
  vtkPVWindow *window = this->Composite->GetWindow();
  
  this->Composite->GetProp()->VisibilityOff();
  window->SetCurrentDataComposite(this->GlyphComposite);
  this->GlyphComposite->GetProp()->VisibilityOn();
  this->GlyphComposite->GetView()->Render();
  window->GetDataList()->Update();
}

void vtkPVGlyph3D::ScaleFactorChanged()
{
  this->Glyph->SetScaleFactor(this->ScaleFactorEntry->GetValueAsFloat());
  this->Glyph->Modified();
  this->Glyph->Update();
  
  this->Composite->GetView()->Render();
}

vtkPVData *vtkPVGlyph3D::GetDataWidget()
{
  if (this->DataWidget == NULL)
    {
    vtkPVPolyData *pd = vtkPVPolyData::New();
    pd->SetPolyData(this->Glyph->GetOutput());
    this->SetDataWidget(pd);
    pd->Delete();    
    }

  return this->DataWidget;
}
