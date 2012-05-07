/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplayInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDisplayInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkToolkits.h"

#if defined(PARAVIEW_USE_X)
# include <X11/Xlib.h>
#endif

vtkStandardNewMacro(vtkPVDisplayInformation);

//----------------------------------------------------------------------------
vtkPVDisplayInformation::vtkPVDisplayInformation()
{
  this->CanOpenDisplay = 1;
}

//----------------------------------------------------------------------------
vtkPVDisplayInformation::~vtkPVDisplayInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CanOpenDisplay: " << this->CanOpenDisplay << endl;
}

//----------------------------------------------------------------------------
bool vtkPVDisplayInformation::CanOpenDisplayLocally()
{
#if defined(PARAVIEW_USE_X)
# if defined(VTK_OPENGL_HAS_OSMESA)
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    return true;
    }
# endif

  Display* dId = XOpenDisplay((char *)NULL);
  if (dId)
    {
    XCloseDisplay(dId);
    return true;
    }
  return false;
#else
  return true;
#endif
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::CopyFromObject(vtkObject*)
{
  this->CanOpenDisplay = vtkPVDisplayInformation::CanOpenDisplayLocally()?1:0;
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::AddInformation(vtkPVInformation* pvi)
{
  vtkPVDisplayInformation* di = vtkPVDisplayInformation::SafeDownCast(pvi);
  if (!di)
    {
    return;
    }
  if (!this->CanOpenDisplay || !di->CanOpenDisplay)
    {
    this->CanOpenDisplay = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->CanOpenDisplay
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDisplayInformation::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &this->CanOpenDisplay);
}
