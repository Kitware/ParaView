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
#include "vtkGlyph3D.h"
#include "vtkPVPolyData.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkKWView.h"
#include "vtkPVAssignment.h"

int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
			int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGlyph3D::vtkPVGlyph3D()
{
  vtkGlyph3D *g;
  
  this->CommandFunction = vtkPVGlyph3DCommand;
  
  g = vtkGlyph3D::New();
  // This default should really be set up looking at the input. 
  g->SetVectorModeToUseNormal();
  this->SetVTKSource(g);
  g->Delete();
}

//----------------------------------------------------------------------------
vtkPVGlyph3D* vtkPVGlyph3D::New()
{
  return new vtkPVGlyph3D();
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::AcceptCallback()
{
  
  if (this->vtkPVSource::GetNthPVInput(1) == NULL)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    vtkPVAssignment *assignment;
    vtkPVPolyDataSource *cone;
    cone = this->GetWindow()->CreateCone();    
    cone->SetName("glyphCone");
    cone->AcceptCallback();
    this->SetSource(cone->GetPVOutput());
    cone->GetPVOutput()->GetActorComposite()->SetVisibility(0);
    pvApp->BroadcastScript("[%s GetActorComposite] SetVisibility 0", 
                           cone->GetPVOutput()->GetTclName());

    // The glyph needs the whole source.
    assignment = cone->GetPVOutput()->GetAssignment();
    assignment->SetPiece(0, 1);
    pvApp->BroadcastScript("%s SetPiece 0 1", assignment->GetTclName());

    }

  this->vtkPVDataSetToPolyDataFilter::AcceptCallback();  
}

//----------------------------------------------------------------------------
void vtkPVGlyph3D::SetSource(vtkPVPolyData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkGlyph3D *f;
  
  f = vtkGlyph3D::SafeDownCast(this->GetVTKSource());
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetSource %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  f->SetSource(pvData->GetPolyData());
  
  this->vtkPVSource::SetNthPVInput(1, pvData);
  if (pvData)
    {
    this->PVInputs[1]->AddPVSourceToUsers(this);
    }
}


//----------------------------------------------------------------------------
void vtkPVGlyph3D::CreateProperties()
{
  // must set the application
  this->vtkPVDataSetToPolyDataFilter::CreateProperties();

  this->AddLabeledToggle("Scaling:", "SetScaling", "GetScaling");
  this->AddLabeledEntry("ScaleFactor:", "SetScaleFactor", "GetScaleFactor");
  this->AddVector2Entry("ScalarRange:", "Min", "Max", "SetRange", "GetRange");
  this->AddLabeledToggle("Clamping:", "SetClamping", "GetClamping");
  this->AddModeList("ScaleMode:", "SetScaleMode", "GetScaleMode");
  this->AddModeListItem("Scalar", 0);
  this->AddModeListItem("Vector", 1);
  this->AddModeListItem("VectorComponents", 2);
  this->AddModeListItem("Off", 3);
  
  this->AddLabeledToggle("Orient:", "SetOrient", "GetOrient");
  this->AddModeList("OrientMode:", "SetVectorMode", "GetVectorMode");
  this->AddModeListItem("Vector", 0);
  this->AddModeListItem("Normal", 1);
  this->AddModeListItem("Off", 2);

  this->AddModeList("ColorMode:", "SetColorMode", "GetColorMode");
  this->AddModeListItem("Scale", 0);
  this->AddModeListItem("Scalar", 1);
  this->AddModeListItem("Vector", 2);

  this->UpdateParameterWidgets();  
}


