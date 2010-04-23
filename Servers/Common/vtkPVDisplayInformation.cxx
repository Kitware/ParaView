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

#if defined(VTK_USE_X)
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
void vtkPVDisplayInformation::CopyFromObject(vtkObject* obj)
{
  // to remove warnings
  obj = obj;

#if defined(VTK_USE_X)
# if defined(VTK_OPENGL_HAS_OSMESA)
  vtkProcessModule* pm = vtkProcessModule::SafeDownCast(obj);
  if (pm->GetOptions()->GetUseOffscreenRendering())
    {
    this->CanOpenDisplay = 1;
    return;
    }
# endif

  Display* dId = XOpenDisplay((char *)NULL); 
  if (dId)
    {
    XCloseDisplay(dId);
    this->CanOpenDisplay = 1;
    }
  else
    {
    this->CanOpenDisplay = 0;
    }
#else
  this->CanOpenDisplay = 1;
#endif
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
