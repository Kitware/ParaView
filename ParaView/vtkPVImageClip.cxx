/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageClip.cxx
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
#include "vtkPVImageClip.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVImage.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkPVAssignment.h"

int vtkPVImageClipCommand(ClientData cd, Tcl_Interp *interp,
			  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageClip::vtkPVImageClip()
{
  this->CommandFunction = vtkPVImageClipCommand;
  
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->ClipXMinEntry = vtkKWEntry::New();
  this->ClipXMinEntry->SetParent(this->Properties);
  this->ClipXMaxEntry = vtkKWEntry::New();
  this->ClipXMaxEntry->SetParent(this->Properties);
  this->ClipYMinEntry = vtkKWEntry::New();
  this->ClipYMinEntry->SetParent(this->Properties);
  this->ClipYMaxEntry = vtkKWEntry::New();
  this->ClipYMaxEntry->SetParent(this->Properties);
  this->ClipZMinEntry = vtkKWEntry::New();
  this->ClipZMinEntry->SetParent(this->Properties);
  this->ClipZMaxEntry = vtkKWEntry::New();
  this->ClipZMaxEntry->SetParent(this->Properties);
  
  this->ClipXMinLabel = vtkKWLabel::New();
  this->ClipXMinLabel->SetParent(this->Properties);
  this->ClipXMaxLabel = vtkKWLabel::New();
  this->ClipXMaxLabel->SetParent(this->Properties);
  this->ClipYMinLabel = vtkKWLabel::New();
  this->ClipYMinLabel->SetParent(this->Properties);
  this->ClipYMaxLabel = vtkKWLabel::New();
  this->ClipYMaxLabel->SetParent(this->Properties);
  this->ClipZMinLabel = vtkKWLabel::New();
  this->ClipZMinLabel->SetParent(this->Properties);
  this->ClipZMaxLabel = vtkKWLabel::New();
  this->ClipZMaxLabel->SetParent(this->Properties);
  
  this->ImageClip = vtkImageClip::New();
  this->ImageClip->ClipDataOn();
}

//----------------------------------------------------------------------------
vtkPVImageClip::~vtkPVImageClip()
{
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ClipXMinEntry->Delete();
  this->ClipXMinEntry = NULL;
  this->ClipXMaxEntry->Delete();
  this->ClipXMaxEntry = NULL;
  this->ClipYMinEntry->Delete();
  this->ClipYMinEntry = NULL;
  this->ClipYMaxEntry->Delete();
  this->ClipYMaxEntry = NULL;
  this->ClipZMinEntry->Delete();
  this->ClipZMinEntry = NULL;
  this->ClipZMaxEntry->Delete();
  this->ClipZMaxEntry = NULL;
  
  this->ClipXMinLabel->Delete();
  this->ClipXMinLabel = NULL;
  this->ClipXMaxLabel->Delete();
  this->ClipXMaxLabel = NULL;
  this->ClipYMinLabel->Delete();
  this->ClipYMinLabel = NULL;
  this->ClipYMaxLabel->Delete();
  this->ClipYMaxLabel = NULL;
  this->ClipZMinLabel->Delete();
  this->ClipZMinLabel = NULL;
  this->ClipZMaxLabel->Delete();
  this->ClipZMaxLabel = NULL;
  
  this->ImageClip->Delete();
  this->ImageClip = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageClip* vtkPVImageClip::New()
{
  return new vtkPVImageClip();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::CreateProperties()
{
  int *extents;
  
  // must set the application
  this->vtkPVSource::CreateProperties();

  extents = this->GetImageClip()->GetOutputWholeExtent();
  
  this->ClipXMinLabel->Create(this->Application, "");
  this->ClipXMinLabel->SetLabel("X Min.:");
  this->ClipXMinEntry->Create(this->Application, "");
  this->ClipXMinEntry->SetValue(extents[0]);
  this->ClipXMaxLabel->Create(this->Application, "");
  this->ClipXMaxLabel->SetLabel("X Max.:");
  this->ClipXMaxEntry->Create(this->Application, "");
  this->ClipXMaxEntry->SetValue(extents[1]);
  this->ClipYMinLabel->Create(this->Application, "");
  this->ClipYMinLabel->SetLabel("Y Min.:");
  this->ClipYMinEntry->Create(this->Application, "");
  this->ClipYMinEntry->SetValue(extents[2]);
  this->ClipYMaxLabel->Create(this->Application, "");
  this->ClipYMaxLabel->SetLabel("Y Max.:");
  this->ClipYMaxEntry->Create(this->Application, "");
  this->ClipYMaxEntry->SetValue(extents[3]);
  this->ClipZMinLabel->Create(this->Application, "");
  this->ClipZMinLabel->SetLabel("Z Min.:");
  this->ClipZMinEntry->Create(this->Application, "");
  this->ClipZMinEntry->SetValue(extents[4]);
  this->ClipZMaxLabel->Create(this->Application, "");
  this->ClipZMaxLabel->SetLabel("Z Max.:");
  this->ClipZMaxEntry->Create(this->Application, "");
  this->ClipZMaxEntry->SetValue(extents[5]);
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ExtentsChanged");
  this->Script("pack %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->ClipXMinLabel->GetWidgetName(),
	       this->ClipXMinEntry->GetWidgetName(),
	       this->ClipXMaxLabel->GetWidgetName(),
	       this->ClipXMaxEntry->GetWidgetName(),
	       this->ClipYMinLabel->GetWidgetName(),
	       this->ClipYMinEntry->GetWidgetName(),
	       this->ClipYMaxLabel->GetWidgetName(),
	       this->ClipYMaxEntry->GetWidgetName(),
	       this->ClipZMinLabel->GetWidgetName(),
	       this->ClipZMinEntry->GetWidgetName(),
	       this->ClipZMaxLabel->GetWidgetName(),
	       this->ClipZMaxEntry->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVImageClip::SetInput(vtkPVImage *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvi->GetTclName());
    }  
  
  this->GetImageClip()->SetInput(pvi->GetImageData());
  this->Input = pvi;
}

//----------------------------------------------------------------------------
void vtkPVImageClip::SetOutput(vtkPVImage *pvi)
{
  this->SetPVData(pvi);

  pvi->SetImageData(this->ImageClip->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageClip::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVImageClip::SetOutputWholeExtent(int xMin, int xMax, 
					  int yMin, int yMax, 
					  int zMin, int zMax)
{  
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutputWholeExtent %d %d %d %d %d %d", 
			   this->GetTclName(), xMin,xMax, yMin,yMax, zMin,zMax);
    }  

  this->ImageClip->SetOutputWholeExtent(xMin, xMax, yMin, yMax, zMin, zMax);
}

//----------------------------------------------------------------------------
void vtkPVImageClip::ExtentsChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVImage *pvi;
  vtkPVActorComposite *ac;
  vtkPVWindow *window = this->GetWindow();
  vtkPVAssignment *a;
  
  this->ImageClip->SetOutputWholeExtent(this->ClipXMinEntry->GetValueAsInt(),
					this->ClipXMaxEntry->GetValueAsInt(),
					this->ClipYMinEntry->GetValueAsInt(),
					this->ClipYMaxEntry->GetValueAsInt(),
					this->ClipZMinEntry->GetValueAsInt(),
					this->ClipZMaxEntry->GetValueAsInt());
  
  if (this->GetPVData() == NULL)
    {
    pvi = vtkPVImage::New();
    pvi->Clone(pvApp);
    pvi->OutlineFlagOff();
    this->SetOutput(pvi);
    a = window->GetPreviousSource()->GetPVData()->GetAssignment();
    pvi->SetAssignment(a);
    this->GetPVData()->GetData()->
      SetUpdateExtent(this->GetPVData()->GetData()->GetWholeExtent());
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  
//  window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
  this->ImageClip->Modified();
  this->ImageClip->Update();
  
  this->GetView()->Render();
  window->GetMainView()->SetSelectedComposite(this);
  window->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::GetSource()
{
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetInput()->GetPVSource());
  this->GetInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}
