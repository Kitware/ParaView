/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPlotMatrixViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPlotMatrixViewProxy.h"
#include "vtkScatterPlotMatrix.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPVPlotMatrixView.h"
#include "vtkPVXMLElement.h"
#include "vtkVector.h"

#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMPlotMatrixViewProxy);

//---------------------------------------------------------------------------
vtkSMPlotMatrixViewProxy::vtkSMPlotMatrixViewProxy()
{
  this->ActiveChanged = false;
}

//---------------------------------------------------------------------------
vtkSMPlotMatrixViewProxy::~vtkSMPlotMatrixViewProxy()
{
  this->SetVTKClassName(0);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    return;
  this->Superclass::CreateVTKObjects();

#if 0 // Server-side animation has been deactivated for now, support
      // for animation is broken in tiling mode, and this 
      // prevent the plot matrix view usage in cs.
      // This class may be removed altogether if we decide not to support
      // it again.
      // see bug 16118
  vtkPVPlotMatrixView *matrix =
      vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
  if (matrix)
    {
//    matrix->AddObserver(vtkCommand::AnnotationChangedEvent, this,
//                        &vtkSMPlotMatrixViewProxy::ActivePlotChanged);
    matrix->AddObserver(vtkCommand::CreateTimerEvent, this,
                        &vtkSMPlotMatrixViewProxy::SendAnimationPath);
    matrix->AddObserver(vtkCommand::AnimationCueTickEvent, this,
                        &vtkSMPlotMatrixViewProxy::AnimationTickEvent);
    }
#endif
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::ActivePlotChanged()
{
  this->ActiveChanged = true;
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::PostRender(bool interactive)
{
  this->Superclass::PostRender(interactive);

  if (this->ActiveChanged)
  {
    vtkPVPlotMatrixView* matrix = vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
    if (matrix)
    {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetActivePlot"
             << matrix->GetActiveRow() << matrix->GetActiveColumn() << vtkClientServerStream::End;
      this->ExecuteStream(stream);
      this->ActiveChanged = false;
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::AnimationTickEvent()
{
  if (this->Session->GetProcessRoles() != vtkPVSession::CLIENT_AND_SERVERS)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "AdvanceAnimationPath"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER);
  }
}

//----------------------------------------------------------------------------
void vtkSMPlotMatrixViewProxy::SendAnimationPath()
{
  // Only send this to the render server(s) if we are not in builtin mode.
  if (this->Session->GetProcessRoles() != vtkPVSession::CLIENT_AND_SERVERS)
  {
    vtkPVPlotMatrixView* matrix = vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
    if (!matrix)
    {
      return;
    }
    vtkScatterPlotMatrix* plotMatrix = vtkScatterPlotMatrix::SafeDownCast(matrix->GetContextItem());
    if (!plotMatrix || plotMatrix->GetNumberOfAnimationPathElements() == 0)
    {
      return;
    }

    vtkIdType n = plotMatrix->GetNumberOfAnimationPathElements();

    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ClearAnimationPath"
           << vtkClientServerStream::End;

    for (vtkIdType i = 0; i < n; ++i)
    {
      vtkVector2i element = plotMatrix->GetAnimationPathElement(i);
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "AddAnimationPath" << element[0]
             << element[1] << vtkClientServerStream::End;
    }

    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "StartAnimationPath"
           << vtkClientServerStream::End;

    this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER);
    this->MarkModified(this);
  }
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkSMPlotMatrixViewProxy::GetContextItem()
{
  vtkPVPlotMatrixView* pvview = vtkPVPlotMatrixView::SafeDownCast(this->GetClientSideObject());
  return pvview ? pvview->GetContextItem() : NULL;
}
