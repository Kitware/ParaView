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
#include "vtkPVImageData.h"
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
  
  this->XDimension = vtkKWLabeledEntry::New();
  this->XDimension->SetParent(this->Properties);
  this->YDimension = vtkKWLabeledEntry::New();
  this->YDimension->SetParent(this->Properties);
  this->ZDimension = vtkKWLabeledEntry::New();
  this->ZDimension->SetParent(this->Properties);
  
  vtkImageReader *r = vtkImageReader::New();
  this->SetImageSource(r);
  r->Delete();
}

//----------------------------------------------------------------------------
vtkPVImageReader::~vtkPVImageReader()
{
  this->Accept->Delete();
  this->Accept = NULL;
  this->Open->Delete();
  this->Open = NULL;
  this->XDimension->Delete();
  this->XDimension = NULL;
  this->YDimension->Delete();
  this->YDimension = NULL;
  this->ZDimension->Delete();
  this->ZDimension = NULL;
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
  
  this->XDimension->Create(this->Application);
  this->XDimension->SetLabel("X Dim.:");
  this->XDimension->SetValue(63);
  this->YDimension->Create(this->Application);
  this->YDimension->SetLabel("Y Dim.:");
  this->YDimension->SetValue(63);
  this->ZDimension->Create(this->Application);
  this->ZDimension->SetLabel("Z Dim.:");
  this->ZDimension->SetValue(93);
  this->Script("pack %s %s %s", this->XDimension->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZDimension->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVImageReader::SetFilePrefix(char *prefix)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->GetImageReader()->SetFilePrefix(prefix);
  
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

  this->GetImageReader()->SetDataByteOrder(o);
  
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

  this->GetImageReader()->SetDataExtent(xmin, xmax, ymin, ymax, zmin, zmax);
  
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

  this->GetImageReader()->SetDataSpacing(sx, sy, sz);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetDataSpacing %f %f %f", 
			   this->GetTclName(), sx, sy, sz);
    }
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartImageReaderProgress(void *arg)
{
  vtkPVImageReader *me = (vtkPVImageReader*)arg;
  me->GetWindow()->SetStatusText("Reading Image");
}

//----------------------------------------------------------------------------
void ImageReaderProgress(void *arg)
{
  vtkPVImageReader *me = (vtkPVImageReader*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetImageReader()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndImageReaderProgress(void *arg)
{
  vtkPVImageReader *me = (vtkPVImageReader*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVImageReader::ImageAccepted()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
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
    this->GetImageReader()->SetStartMethod(StartImageReaderProgress, this);
    this->GetImageReader()->SetProgressMethod(ImageReaderProgress, this);
    this->GetImageReader()->SetEndMethod(EndImageReaderProgress, this);
    this->InitializeData();
    }
  
  if (window->GetPreviousSource() != NULL)
    {
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    }
    
  window->GetMainView()->SetSelectedComposite(this);
  window->GetMainView()->ResetCamera();
  this->GetView()->Render();
  window->GetSourceList()->Update();
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

//----------------------------------------------------------------------------
vtkImageReader *vtkPVImageReader::GetImageReader()
{
  return vtkImageReader::SafeDownCast(this->ImageSource);  
}
