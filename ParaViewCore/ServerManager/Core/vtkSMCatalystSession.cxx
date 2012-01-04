/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCatalystSession.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMCatalystSession);
vtkCxxSetObjectMacro(vtkSMCatalystSession, VisualizationSession, vtkSMSession);
//----------------------------------------------------------------------------
vtkSMCatalystSession::vtkSMCatalystSession(): Superclass(false),
  InsituPort(0),
  VisualizationSession(0)
{
}

//----------------------------------------------------------------------------
vtkSMCatalystSession::~vtkSMCatalystSession()
{
  this->SetVisualizationSession(NULL);
}

//----------------------------------------------------------------------------
void vtkSMCatalystSession::Initialize()
{
  assert (this->VisualizationSession != NULL);

  this->Superclass::Initialize();

  assert(this->GetProcessRoles() & vtkPVSession::CLIENT);

  // Create the catalyst adaptor proxy on the visualization session. This proxy
  // does all the grunt work.
  vtkSMSessionProxyManager* pxm = this->VisualizationSession->GetSessionProxyManager();
  vtkSMProxy* adaptor = pxm->NewProxy("coprocessing", "LiveInsituLink");
  if (!adaptor)
    {
    vtkErrorMacro("Current VisualizationSession cannot create LiveInsituLink.");
    return;
    }

  vtkSMPropertyHelper(adaptor, "InsituPort").Set(this->InsituPort);
  vtkSMPropertyHelper(adaptor, "ProcessType").Set("Visualization");
  adaptor->UpdateVTKObjects();
  this->LiveInsituLink.TakeReference(adaptor);
  // adaptor creates a vtkLiveInsituLink instance on the data-server.

  adaptor->InvokeCommand("Initialize");
}

//----------------------------------------------------------------------------
void vtkSMCatalystSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
