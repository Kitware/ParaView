/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRampKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVRampKeyFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"

vtkStandardNewMacro(vtkPVRampKeyFrame);
vtkCxxRevisionMacro(vtkPVRampKeyFrame, "1.1");

//-----------------------------------------------------------------------------
vtkPVRampKeyFrame::vtkPVRampKeyFrame()
{
  this->SetKeyFrameProxyXMLName("RampKeyFrame");
}

//-----------------------------------------------------------------------------
vtkPVRampKeyFrame::~vtkPVRampKeyFrame()
{
}

//-----------------------------------------------------------------------------
void vtkPVRampKeyFrame::ChildCreate(vtkKWApplication* app)
{
  this->Superclass::ChildCreate(app);
}

//-----------------------------------------------------------------------------
void vtkPVRampKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
