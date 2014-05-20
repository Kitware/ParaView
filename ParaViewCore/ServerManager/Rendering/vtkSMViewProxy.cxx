/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVOptions.h"
#include "vtkPVView.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSMUtilities.h"
#include "vtkSmartPointer.h"

#include <assert.h>

namespace
{
  const char* vtkGetRepresentationNameFromHints(
    const char* viewType, vtkPVXMLElement* hints, int port)
    {
    if (!hints)
      {
      return NULL;
      }

    for (unsigned int cc=0, max=hints->GetNumberOfNestedElements(); cc<max; ++cc)
      {
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      if (child == NULL || child->GetName() == NULL)
        {
        continue;
        }

      // LEGACY: support DefaultRepresentations hint.
      // <Hints>
      //    <DefaultRepresentations representation="Foo" />
      // </Hints>
      if (strcmp(child->GetName(), "DefaultRepresentations") == 0)
        {
        return child->GetAttribute("representation");
        }

      // <Hints>
      //    <Representation port="outputPort" view="ViewName" type="ReprName" />
      // </Hints>
      else if (strcmp(child->GetName(), "Representation") == 0 &&
        // has an attribute "view" that matches the viewType.
        child->GetAttribute("view") &&
        strcmp(child->GetAttribute("view"), viewType) == 0)
        {
        // if port is present, it must match "port".
        int xmlPort;
        if (!child->GetScalarAttribute("port", &xmlPort) == 0 ||
          xmlPort == port)
          {
          return child->GetAttribute("type");
          }
        }
      }
    return NULL;
    }
};

vtkStandardNewMacro(vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
{
  this->SetLocation(vtkProcessModule::CLIENT_AND_SERVERS);
  this->DefaultRepresentationName = 0;
  this->Enable = true;
}

//----------------------------------------------------------------------------
vtkSMViewProxy::~vtkSMViewProxy()
{
  this->SetDefaultRepresentationName(0);
}

//----------------------------------------------------------------------------
vtkView* vtkSMViewProxy::GetClientSideView()
{
  if (this->ObjectsCreated)
    {
    return vtkView::SafeDownCast(this->GetClientSideObject());
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go further...
  if(this->Location == 0)
    {
    return;
    }

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "Initialize"
         << static_cast<int>(this->GetGlobalID())
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  vtkObject::SafeDownCast(this->GetClientSideObject())->AddObserver(
      vtkPVView::ViewTimeChangedEvent,
      this, &vtkSMViewProxy::ViewTimeChanged);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ViewTimeChanged()
{
  vtkSMPropertyHelper helper1(this, "Representations");
  for (unsigned int cc=0; cc  < helper1.GetNumberOfElements(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      helper1.GetAsProxy(cc));
    if (repr)
      {
      repr->ViewTimeChanged();
      }
    }

  vtkSMPropertyHelper helper2(this, "HiddenRepresentations", true);
  for (unsigned int cc=0; cc  < helper2.GetNumberOfElements(); cc++)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      helper2.GetAsProxy(cc));
    if (repr)
      {
      repr->ViewTimeChanged();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::StillRender()
{
  // bug 0013947
  // on Mac OSX don't render into invalid drawable, all subsequent
  // OpenGL calls fail with invalid framebuffer operation.
  if (this->IsContextReadyForRendering() == false)
    {
    return;
    }

  int interactive = 0;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  this->GetSession()->PrepareProgress();
  // We call update separately from the render. This is done so that we don't
  // get any synchronization issues with GUI responding to the data-updated
  // event by making some data information requests(for example). If those
  // happen while StillRender/InteractiveRender is being executed on the server
  // side then we get deadlocks.
  this->Update();

  vtkTypeUInt32 render_location = this->PreRender(interactive==1);

  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "StillRender"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, render_location);
    }

  this->PostRender(interactive==1);
  this->GetSession()->CleanupPendingProgress();
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InteractiveRender()
{
  int interactive = 1;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  this->GetSession()->PrepareProgress();

  // Interactive render will not call Update() at all. It's expected that you
  // must have either called a StillRender() or an Update() before triggering an
  // interactive render. This is critical to keep interactive rates fast when
  // working over a slow client-server connection.
  // this->Update();

  vtkTypeUInt32 render_location = this->PreRender(interactive==1);

  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "InteractiveRender"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, render_location);
    }

  this->PostRender(interactive==1);
  this->GetSession()->CleanupPendingProgress();
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::Update()
{
  if (this->ObjectsCreated && this->NeedsUpdate)
    {
    vtkClientServerStream stream;

    // To avoid race conditions in multi-client modes, we are taking a peculiar
    // approach. Any ivar that affect parallel communication are overridden
    // using the client-side values in the same ExecuteStream() call. That
    // ensures that two clients cannot enter race condition. This results in minor
    // increase in the size of the messages sent, but overall the benefits are
    // greater.
    vtkPVView* pvview = vtkPVView::SafeDownCast(this->GetClientSideObject());
    if (pvview)
      {
      int use_cache =  pvview->GetUseCache()? 1 : 0;
      stream << vtkClientServerStream::Invoke
             << VTKOBJECT(this)
             << "SetUseCache" << use_cache
             << vtkClientServerStream::End;
      }
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "Update"
           << vtkClientServerStream::End;
    this->GetSession()->PrepareProgress();
    this->ExecuteStream(stream);
    this->GetSession()->CleanupPendingProgress();

    unsigned int numProducers = this->GetNumberOfProducers();
    for (unsigned int i=0; i<numProducers; i++)
      {
      vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
        this->GetProducerProxy(i));
      if (repr)
        {
        repr->ViewUpdated(this);
        }
      else
        {
        //this->GetProducerProxy(i)->PostUpdateData();
        }
      }

    this->PostUpdateData();
    }
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* proxy, int outputPort)
{
  assert("The session should be valid" && this->Session);

  vtkSMSourceProxy* producer = vtkSMSourceProxy::SafeDownCast(proxy);
  if (
    (producer == NULL) ||
    (outputPort < 0) ||
    (static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort) ||
    (producer->GetSession() != this->GetSession()))
    {
    return NULL;
    }

  // Update with time from the view to ensure we have up-to-date data.
  double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
  producer->UpdatePipeline(view_time);

  const char* representationType =
    this->GetRepresentationType(producer, outputPort);
  if (!representationType)
    {
    return NULL;
    }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  vtkSmartPointer<vtkSMProxy> p;
  p.TakeReference(pxm->NewProxy("representations", representationType));
  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(p);
  if (repr)
    {
    repr->Register(this);
    return repr;
    }
  vtkWarningMacro("Failed to create representation (representations,"
    << representationType <<").");
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkSMViewProxy::GetRepresentationType(
  vtkSMSourceProxy* producer, int outputPort)
{
  assert(producer &&
         static_cast<int>(producer->GetNumberOfOutputPorts()) > outputPort);

  // Process producer hints to see if indicates what type of representation
  // to create for this view.
  if (const char* reprName = vtkGetRepresentationNameFromHints(
      this->GetXMLName(), producer->GetHints(), outputPort))
    {
    return reprName;
    }

  // check if we have default representation name specified in XML.
  if (this->DefaultRepresentationName)
    {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    vtkSMProxy* prototype = pxm->GetPrototypeProxy(
      "representations", this->DefaultRepresentationName);
    if (prototype)
      {
      vtkSMProperty* inputProp = prototype->GetProperty("Input");
      vtkSMUncheckedPropertyHelper helper(inputProp);
      helper.Set(producer, outputPort);
      bool acceptable = (inputProp->IsInDomains() > 0);
      helper.SetNumberOfElements(0);

      if (acceptable)
        {
        return this->DefaultRepresentationName;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::CanDisplayData(vtkSMSourceProxy* producer, int outputPort)
{
  if (producer == NULL || outputPort < 0 ||
      static_cast<int>(producer->GetNumberOfOutputPorts()) <= outputPort ||
      producer->GetSession() != this->GetSession())
    {
    return false;
    }

  const char* type = this->GetRepresentationType(producer, outputPort);
  if (type != NULL)
    {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    return (pxm->GetPrototypeProxy("representations", type) != NULL);
    }

  return false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMViewProxy::FindRepresentation(
  vtkSMSourceProxy* producer, int outputPort)
{
  vtkSMPropertyHelper helper (this, "Representations");
  for (unsigned int cc=0, max=helper.GetNumberOfElements(); cc < max; ++cc)
    {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(helper.GetAsProxy(cc));
    if (repr &&
      repr->GetProperty("Input"))
      {
      vtkSMPropertyHelper helper2(repr, "Input");
      if (helper2.GetAsProxy() == producer &&
          static_cast<int>(helper2.GetOutputPort()) == outputPort)
        {
        return repr;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMViewProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }

  const char* repr_name = element->GetAttribute("representation_name");
  if (repr_name)
    {
    this->SetDefaultRepresentationName(repr_name);
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewProxy::CaptureWindow(int magnification)
{
  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "PrepareForScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  vtkImageData* capture = this->CaptureWindowInternal(magnification);

  if (this->ObjectsCreated)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "CleanupAfterScreenshot"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  if (capture)
    {
    int position[2];
    vtkSMPropertyHelper(this, "ViewPosition").Get(position, 2);

    // Update image extents based on ViewPosition
    int extents[6];
    capture->GetExtent(extents);
    for (int cc=0; cc < 4; cc++)
      {
      extents[cc] += position[cc/2]*magnification;
      }
    capture->SetExtent(extents);
    }
  return capture;
}

//-----------------------------------------------------------------------------
int vtkSMViewProxy::WriteImage(const char* filename,
  const char* writerName, int magnification)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));

  if (vtkProcessModule::GetProcessModule()->GetOptions()->GetSymmetricMPIMode())
    {
    return vtkSMUtilities::SaveImageOnProcessZero(shot, filename, writerName);
    }
  return vtkSMUtilities::SaveImage(shot, filename, writerName);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMViewProxy::IsContextReadyForRendering()
{
  if (vtkRenderWindow* window = this->GetRenderWindow())
    {
    return window->IsDrawable();
    }
  return true;
}
