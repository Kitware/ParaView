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
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"
#include "vtkPVImage.h"

int vtkPVImageClipCommand(ClientData cd, Tcl_Interp *interp,
			  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageClip::vtkPVImageClip()
{
  this->CommandFunction = vtkPVImageClipCommand;
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  
  this->ClipXMinEntry = vtkKWEntry::New();
  this->ClipXMinEntry->SetParent(this);
  this->ClipXMaxEntry = vtkKWEntry::New();
  this->ClipXMaxEntry->SetParent(this);
  this->ClipYMinEntry = vtkKWEntry::New();
  this->ClipYMinEntry->SetParent(this);
  this->ClipYMaxEntry = vtkKWEntry::New();
  this->ClipYMaxEntry->SetParent(this);
  this->ClipZMinEntry = vtkKWEntry::New();
  this->ClipZMinEntry->SetParent(this);
  this->ClipZMaxEntry = vtkKWEntry::New();
  this->ClipZMaxEntry->SetParent(this);
  
  this->ClipXMinLabel = vtkKWLabel::New();
  this->ClipXMinLabel->SetParent(this);
  this->ClipXMaxLabel = vtkKWLabel::New();
  this->ClipXMaxLabel->SetParent(this);
  this->ClipYMinLabel = vtkKWLabel::New();
  this->ClipYMinLabel->SetParent(this);
  this->ClipYMaxLabel = vtkKWLabel::New();
  this->ClipYMaxLabel->SetParent(this);
  this->ClipZMinLabel = vtkKWLabel::New();
  this->ClipZMinLabel->SetParent(this);
  this->ClipZMaxLabel = vtkKWLabel::New();
  this->ClipZMaxLabel->SetParent(this);
  
  this->ImageClip = vtkImageClip::New();
}

//----------------------------------------------------------------------------
vtkPVImageClip::~vtkPVImageClip()
{ 
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
int vtkPVImageClip::Create(char *args)
{
  int *extents;
  
  // must set the application
  if (this->vtkPVSource::Create(args) == 0)
    {
    return 0;
    }
  
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
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ExtentsChanged");
  this->Script("pack %s %s %s %s %s %s %s %s %s %s %s %s %s",
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
  return 1;
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
void vtkPVImageClip::ExtentsChanged()
{  
  this->ImageClip->SetOutputWholeExtent(this->ClipXMinEntry->GetValueAsInt(),
					this->ClipXMaxEntry->GetValueAsInt(),
					this->ClipYMinEntry->GetValueAsInt(),
					this->ClipYMaxEntry->GetValueAsInt(),
					this->ClipZMinEntry->GetValueAsInt(),
					this->ClipZMaxEntry->GetValueAsInt());
  this->ImageClip->Modified();
  this->ImageClip->Update();
  
  this->Composite->GetView()->Render();
}
