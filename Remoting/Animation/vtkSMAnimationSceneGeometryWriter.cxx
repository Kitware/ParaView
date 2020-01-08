/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneGeometryWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneGeometryWriter.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMAnimationSceneGeometryWriter);
vtkCxxSetObjectMacro(vtkSMAnimationSceneGeometryWriter, ViewModule, vtkSMProxy);

//-----------------------------------------------------------------------------
vtkSMAnimationSceneGeometryWriter::vtkSMAnimationSceneGeometryWriter()
{
  this->GeometryWriter = 0;
  this->ViewModule = 0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneGeometryWriter::~vtkSMAnimationSceneGeometryWriter()
{
  this->SetViewModule(0);
  if (this->GeometryWriter)
  {
    this->GeometryWriter->Delete();
    this->GeometryWriter = 0;
  }
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneGeometryWriter::SaveInitialize(int vtkNotUsed(startCount))
{
  if (!this->ViewModule)
  {
    vtkErrorMacro("No view from which to save the geometry is set.");
    return false;
  }

  assert("The session should be set by now" && this->Session);

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  this->GeometryWriter = pxm->NewProxy("writers", "XMLPVAnimationWriter");

  vtkSMPropertyHelper(this->GeometryWriter, "FileName").Set(this->FileName);

  vtkSMProxyProperty* pp =
    vtkSMProxyProperty::SafeDownCast(this->ViewModule->GetProperty("Representations"));

  vtkSMProxyProperty* gwInput =
    vtkSMProxyProperty::SafeDownCast(this->GeometryWriter->GetProperty("Representations"));
  gwInput->RemoveAllProxies();

  for (unsigned int cc = 0; cc < pp->GetNumberOfProxies(); ++cc)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(pp->GetProxy(cc));
    if (repr && vtkSMPropertyHelper(repr, "Visibility", true).GetAsInt() != 0)
    {
      gwInput->AddProxy(repr);
    }
  }
  this->GeometryWriter->UpdateVTKObjects();
  this->GeometryWriter->InvokeCommand("Start");
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneGeometryWriter::SaveFrame(double time)
{
  vtkSMPropertyHelper(this->GeometryWriter, "WriteTime").Set(time);
  this->GeometryWriter->UpdateProperty("WriteTime", 1);
  this->GeometryWriter->UpdatePropertyInformation();
  if (vtkSMPropertyHelper(this->GeometryWriter, "ErrorCode").GetAsInt())
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneGeometryWriter::SaveFinalize()
{
  if (!this->GeometryWriter)
  {
    return true;
  }

  this->GeometryWriter->InvokeCommand("Finish");
  this->GeometryWriter->Delete();
  this->GeometryWriter = 0;
  return true;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneGeometryWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ViewModule: " << this->ViewModule << endl;
}
