/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBooleanKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVBooleanKeyFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"

vtkStandardNewMacro(vtkPVBooleanKeyFrame);
vtkCxxRevisionMacro(vtkPVBooleanKeyFrame, "1.1");

//-----------------------------------------------------------------------------
vtkPVBooleanKeyFrame::vtkPVBooleanKeyFrame()
{
  this->SetKeyFrameProxyXMLName("BooleanKeyFrame");
}

//-----------------------------------------------------------------------------
vtkPVBooleanKeyFrame::~vtkPVBooleanKeyFrame()
{
}

//-----------------------------------------------------------------------------
void vtkPVBooleanKeyFrame::ChildCreate(vtkKWApplication* app)
{
  this->Superclass::ChildCreate(app);
}

//-----------------------------------------------------------------------------
void vtkPVBooleanKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

