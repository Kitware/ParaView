/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageReader.cxx
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

#include "vtkPVImageReader.h"
#include "vtkPVApplication.h"
#include "vtkPVImage.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

//----------------------------------------------------------------------------
vtkPVImageReader::vtkPVImageReader()
{
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->Open = vtkKWPushButton::New();
  this->Open->SetParent(this->Properties);
  
  this->XLabel = vtkKWLabel::New();
  this->XLabel->SetParent(this->Properties);
  this->XDimension = vtkKWEntry::New();
  this->XDimension->SetParent(this->Properties);
  this->YLabel = vtkKWLabel::New();
  this->YLabel->SetParent(this->Properties);
  this->YDimension = vtkKWEntry::New();
  this->YDimension->SetParent(this->Properties);
  this->ZLabel = vtkKWLabel::New();
  this->ZLabel->SetParent(this->Properties);
  this->ZDimension = vtkKWEntry::New();
  this->ZDimension->SetParent(this->Properties);
  
  this->ImageReader = vtkImageReader::New();
}

//----------------------------------------------------------------------------
vtkPVImageReader::~vtkPVImageReader()
{
  this->Accept->Delete();
  this->Accept = NULL;
  this->Open->Delete();
  this->Open = NULL;
  this->XLabel->Delete();
  this->XLabel = NULL;
  this->XDimension->Delete();
  this->XDimension = NULL;
  this->YLabel->Delete();
  this->YLabel = NULL;
  this->YDimension->Delete();
  this->YDimension = NULL;
  this->ZLabel->Delete();
  this->ZLabel = NULL;
  this->ZDimension->Delete();
  this->ZDimension = NULL;
  
  this->ImageReader->Delete();
  this->ImageReader = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageReader* vtkPVImageReader::New()
{
  return new vtkPVImageReader();
}

//----------------------------------------------------------------------------
void vtkPVImageReader::CreateProperties()
{  
  this->vtkPVSource::CreateProperties();
  
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ImageAccepted");
  this->Script("pack %s", this->Accept->GetWidgetName());
  
  this->Open->Create(this->Application, "-text OpenFile");
  this->Open->SetCommand(this, "OpenFile");
  this->Script("pack %s", this->Open->GetWidgetName());
  
  this->XLabel->Create(this->Application, "");
  this->XLabel->SetLabel("X Dim.");
  this->XDimension->Create(this->Application, "");
  this->XDimension->SetValue(63);
  this->YLabel->Create(this->Application, "");
  this->YLabel->SetLabel("Y Dim.");
  this->YDimension->Create(this->Application, "");
  this->YDimension->SetValue(63);
  this->ZLabel->Create(this->Application, "");
  this->ZLabel->SetLabel("Z Dim.");
  this->ZDimension->Create(this->Application, "");
  this->ZDimension->SetValue(93);
  this->Script("pack %s %s %s %s %s %s", this->XLabel->GetWidgetName(),
	       this->XDimension->GetWidgetName(),
	       this->YLabel->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZLabel->GetWidgetName(),
	       this->ZDimension->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVImageReader::SetFilePrefix(char *prefix)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->ImageReader->SetFilePrefix(prefix);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetFilePrefix %s", 
			   this->GetTclName(), prefix);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageReader::SetDataByteOrder(int o)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->ImageReader->SetDataByteOrder(o);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetDataByteOrder %d", this->GetTclName(), o);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageReader::SetDataExtent(int xmin,int xmax, int ymin,int ymax, 
				     int zmin,int zmax)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->ImageReader->SetDataExtent(xmin, xmax, ymin, ymax, zmin, zmax);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetDataExtent %d %d %d %d %d %d",
		   this->GetTclName(), xmin,xmax, ymin,ymax, zmin,zmax);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageReader::SetDataSpacing(float sx, float sy, float sz)
{
 vtkPVApplication *pvApp = this->GetPVApplication();

  this->ImageReader->SetDataSpacing(sx, sy, sz);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetDataSpacing %f %f %f", 
			   this->GetTclName(), sx, sy, sz);
    }
}


//----------------------------------------------------------------------------
void vtkPVImageReader::SetOutput(vtkPVImage *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetPVData(pvi);
  pvi->SetData(this->ImageReader->GetOutput());
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(), 
			   pvi->GetTclName());
    }
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageReader::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVImageReader::ImageAccepted()
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  vtkPVImage *pvImage;
  vtkPVAssignment *a;
  vtkPVActorComposite *ac;
  vtkPVWindow *window = this->GetWindow();
  
  // Setup the image reader from the UI.
  this->SetDataByteOrder(VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN);
  this->SetDataExtent(0, this->XDimension->GetValueAsInt(),
		      0, this->YDimension->GetValueAsInt(),
		      1, this->ZDimension->GetValueAsInt());
  this->SetDataSpacing(4, 4, 1.8);
  // This will have to be changed when general files are used.
  // There should be a label/entry/button that has the prefix.
  this->SetFilePrefix("../../vtkdata/headsq/quarter");

  if (this->GetPVData() == NULL)
    {
    pvImage = vtkPVImage::New();
    pvImage->Clone(pvApp);
    a = vtkPVAssignment::New();
    a->Clone(pvApp);
    
    this->SetOutput(pvImage);
    // It is important that the pvImage have its image data befor this call.
    // The assignments vtk object is vtkPVExtentTranslator which needs the image.
    // This may get resolved as more functionality of Assignement gets into ExtentTranslator.
    // Maybe the pvImage should create the vtkIamgeData, and the source will set it as its output.
    a->SetOriginalImage(pvImage);
    
    pvImage->SetAssignment(a);
    
    this->CreateDataPage();
  
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  
  if (window->GetPreviousSource() != NULL)
    {
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    }
  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
  window->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVImageReader::OpenFile()
{
  char *path;
  
  // We need to figure out what to do if the image is stored in multiple files
  // (so we only need a file prefix, not a file).
  this->Script("tk_getOpenFile -title \"Open Image File\"");
  
  path = 
    strcpy(new char[strlen(this->Application->GetMainInterp()->result)+1], 
	   this->Application->GetMainInterp()->result);
}
