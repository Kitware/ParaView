/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIceTFactory.h"
#include "vtkVersion.h"

#ifdef USE_CR
#include "vtkCrOpenGLRenderer.h"
#include "vtkCrOpenGLRenderWindow.h"
#endif

vtkCxxRevisionMacro(vtkIceTFactory, "1.3");

VTK_FACTORY_INTERFACE_IMPLEMENT(vtkIceTFactory);

vtkIceTFactory* vtkIceTFactory::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIceTFactory");
  if(ret)
    {
    return (vtkIceTFactory*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkIceTFactory;
}

void vtkIceTFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "VTK-SNL parallel object factory" << endl;
}

#ifdef USE_CR
VTK_CREATE_CREATE_FUNCTION(vtkCrOpenGLRenderer);
VTK_CREATE_CREATE_FUNCTION(vtkCrOpenGLRenderWindow);
#endif

vtkIceTFactory::vtkIceTFactory()
{
#ifdef USE_CR
  this->RegisterOverride("vtkRenderer",
       "vtkCrOpenGLRenderer",
       "Chromium",
#  ifdef USE_ICET
       0, // Disable this override if Ice-T is in use
#  else
       1, // Otherwise, enable the Chromium renderers
#  endif
       vtkObjectFactoryCreatevtkCrOpenGLRenderer);
  this->RegisterOverride("vtkRenderWindow",
       "vtkCrOpenGLRenderWindow",
       "Chromium",
#  ifdef USE_ICET
       0, // Disable this override if Ice-T is in use
#  else
       1, // Otherwise, enable the Chromium renderers
#  endif
       vtkObjectFactoryCreatevtkCrOpenGLRenderWindow);
#endif //USE_CR
}

const char *vtkIceTFactory::GetVTKSourceVersion()
{
  return VTK_SOURCE_VERSION;
}

const char *vtkIceTFactory::GetDescription()
{
  return "SNL Support Factory for VTK";
}
