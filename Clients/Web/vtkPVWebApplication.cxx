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
#include "vtkPVWebApplication.h"

#include "vtkBase64Utilities.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataEncoder.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPVRenderView.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGLExporter.h"
#include "vtkWebGLObject.h"
#include "vtkWebInteractionEvent.h"

#include <assert.h>
#include <cmath>
#include <map>

class vtkPVWebApplication::vtkInternals
{
public:
  struct ImageCacheValueType
  {
  public:
    vtkSmartPointer<vtkUnsignedCharArray> Data;
    bool NeedsRender;
    bool HasImagesBeingProcessed;
    vtkObject* ViewPointer;
    unsigned long ObserverId;
    ImageCacheValueType()
      : NeedsRender(true)
      , HasImagesBeingProcessed(false)
      , ViewPointer(nullptr)
      , ObserverId(0)
    {
    }

    void SetListener(vtkObject* view)
    {
      if (this->ViewPointer == view)
      {
        return;
      }

      if (this->ViewPointer && this->ObserverId)
      {
        this->ViewPointer->RemoveObserver(this->ObserverId);
        this->ObserverId = 0;
      }
      this->ViewPointer = view;
      if (this->ViewPointer)
      {
        this->ObserverId = this->ViewPointer->AddObserver(
          vtkCommand::AnyEvent, this, &ImageCacheValueType::ViewEventListener);
      }
    }

    void RemoveListener(vtkObject* view)
    {
      if (this->ViewPointer && this->ViewPointer == view && this->ObserverId)
      {
        this->ViewPointer->RemoveObserver(this->ObserverId);
        this->ObserverId = 0;
        this->ViewPointer = nullptr;
      }
    }

    void ViewEventListener(vtkObject*, unsigned long, void*) { this->NeedsRender = true; }
  };
  typedef std::map<void*, ImageCacheValueType> ImageCacheType;
  ImageCacheType ImageCache;

  typedef std::map<void*, unsigned int> ButtonStatesType;
  ButtonStatesType ButtonStates;

  vtkNew<vtkDataEncoder> Encoder;

  // WebGL related struct
  struct WebGLObjCacheValue
  {
  public:
    int ObjIndex;
    std::map<int, std::string> BinaryParts;
  };
  // map for <vtkWebGLExporter, <webgl-objID, WebGLObjCacheValue> >
  typedef std::map<std::string, WebGLObjCacheValue> WebGLObjId2IndexMap;
  std::map<vtkWebGLExporter*, WebGLObjId2IndexMap> WebGLExporterObjIdMap;
  // map for <vtkSMViewProxy, vtkWebGLExporter>
  std::map<vtkSMViewProxy*, vtkSmartPointer<vtkWebGLExporter> > ViewWebGLMap;
  std::string LastAllWebGLBinaryObjects;
};

vtkStandardNewMacro(vtkPVWebApplication);
//----------------------------------------------------------------------------
vtkPVWebApplication::vtkPVWebApplication()
  : ImageEncoding(ENCODING_BASE64)
  , ImageCompression(COMPRESSION_JPEG)
  , Internals(new vtkPVWebApplication::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVWebApplication::~vtkPVWebApplication()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVWebApplication::GetHasImagesBeingProcessed(vtkSMViewProxy* view)
{
  const vtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];
  return value.HasImagesBeingProcessed;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkPVWebApplication::InteractiveRender(vtkSMViewProxy* view, int quality)
{
  // for now, just do the same as StillRender().
  return this->StillRender(view, quality);
}

//----------------------------------------------------------------------------
void vtkPVWebApplication::InvalidateCache(vtkSMViewProxy* view)
{
  this->Internals->ImageCache[view].NeedsRender = true;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkPVWebApplication::StillRender(vtkSMViewProxy* view, int quality)
{
  if (!view)
  {
    vtkErrorMacro("No view specified.");
    return nullptr;
  }
  // threading can be tricky to debug -
  bool doThread = true;

  vtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];
  value.SetListener(view);

  if (value.NeedsRender == false && value.Data != nullptr && view->GetNeedsUpdate() == false)
  {
    // cout <<  "Reusing cache" << endl;
    if (doThread)
    {
      bool latest = this->Internals->Encoder->GetLatestOutput(view->GetGlobalID(), value.Data);
      value.HasImagesBeingProcessed = !latest;
    }
    else
    {
      value.HasImagesBeingProcessed = false;
    }
    return value.Data;
  }

  // cout <<  "Regenerating " << endl;
  // vtkTimerLog::ResetLog();
  // vtkTimerLog::CleanupLog();
  // vtkTimerLog::MarkStartEvent("StillRenderToString");
  // vtkTimerLog::MarkStartEvent("CaptureWindow");

  // TODO: We should add logic to check if a new rendering needs to be done and
  // then alone do a new rendering otherwise use the cached image.
  vtkImageData* image = view->CaptureWindow(1);
  image->GetDimensions(this->LastStillRenderImageSize);
  // vtkTimerLog::MarkEndEvent("CaptureWindow");

  // vtkTimerLog::MarkEndEvent("StillRenderToString");
  // vtkTimerLog::DumpLogWithIndents(&cout, 0.0);

  if (doThread || this->ImageEncoding)
  {
    this->Internals->Encoder->PushAndTakeReference(
      view->GetGlobalID(), image, quality, this->ImageEncoding);
    assert(image == nullptr);

    if (value.Data == nullptr)
    {
      // we need to wait till output is processed.
      // cout << "Flushing" << endl;
      this->Internals->Encoder->Flush(view->GetGlobalID());
      // cout << "Done Flushing" << endl;
    }

    bool latest = this->Internals->Encoder->GetLatestOutput(view->GetGlobalID(), value.Data);
    value.HasImagesBeingProcessed = !latest;
  }
  else
  {
    vtkNew<vtkJPEGWriter> writer;
    writer->WriteToMemoryOn();
    writer->SetInputData(image);
    writer->SetQuality(quality);
    writer->Write();
    // smart pointer does the right thing, even if Data was null
    value.Data = writer->GetResult();
  }
  value.NeedsRender = false;
  return value.Data;
}

//----------------------------------------------------------------------------
const char* vtkPVWebApplication::StillRenderToString(
  vtkSMViewProxy* view, unsigned long time, int quality)
{
  vtkUnsignedCharArray* array = this->StillRender(view, quality);
  if (array && array->GetMTime() != time)
  {
    this->LastStillRenderToMTime = array->GetMTime();
    // cout << "Image size: " << array->GetNumberOfTuples() << endl;
    return reinterpret_cast<char*>(array->GetPointer(0));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkPVWebApplication::StillRenderToBuffer(
  vtkSMViewProxy* view, unsigned long time, int quality)
{
  vtkUnsignedCharArray* array = this->StillRender(view, quality);
  if (array && array->GetMTime() != time)
  {
    this->LastStillRenderToMTime = array->GetMTime();
    return array;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVWebApplication::HandleInteractionEvent(
  vtkSMViewProxy* view, vtkWebInteractionEvent* event)
{
  vtkRenderWindowInteractor* iren = nullptr;

  if (view->GetRenderWindow())
  {
    iren = view->GetRenderWindow()->GetInteractor();
  }
  else
  {
    vtkErrorMacro("Interaction not supported for view : " << view);
    return false;
  }

  int viewSize[2];
  vtkSMPropertyHelper(view, "ViewSize").Get(viewSize, 2);

  int posX = std::floor(viewSize[0] * event->GetX() + 0.5);
  int posY = std::floor(viewSize[1] * event->GetY() + 0.5);

  int ctrlKey = (event->GetModifiers() & vtkWebInteractionEvent::CTRL_KEY) != 0 ? 1 : 0;
  int shiftKey = (event->GetModifiers() & vtkWebInteractionEvent::SHIFT_KEY) != 0 ? 1 : 0;
  iren->SetEventInformation(posX, posY, ctrlKey, shiftKey, event->GetKeyCode());

  unsigned int prev_buttons = this->Internals->ButtonStates[view];
  unsigned int changed_buttons = (event->GetButtons() ^ prev_buttons);
  iren->MouseMoveEvent();
  if ((changed_buttons & vtkWebInteractionEvent::LEFT_BUTTON) != 0)
  {
    if ((event->GetButtons() & vtkWebInteractionEvent::LEFT_BUTTON) != 0)
    {
      iren->LeftButtonPressEvent();
    }
    else
    {
      iren->LeftButtonReleaseEvent();
    }
  }

  if ((changed_buttons & vtkWebInteractionEvent::RIGHT_BUTTON) != 0)
  {
    if ((event->GetButtons() & vtkWebInteractionEvent::RIGHT_BUTTON) != 0)
    {
      iren->RightButtonPressEvent();
    }
    else
    {
      iren->RightButtonReleaseEvent();
    }
  }
  if ((changed_buttons & vtkWebInteractionEvent::MIDDLE_BUTTON) != 0)
  {
    if ((event->GetButtons() & vtkWebInteractionEvent::MIDDLE_BUTTON) != 0)
    {
      iren->MiddleButtonPressEvent();
    }
    else
    {
      iren->MiddleButtonReleaseEvent();
    }
  }

  this->Internals->ButtonStates[view] = event->GetButtons();

  bool needs_render = (changed_buttons != 0 || event->GetButtons());
  this->Internals->ImageCache[view].NeedsRender = needs_render;
  return needs_render;
}

// ---------------------------------------------------------------------------
const char* vtkPVWebApplication::GetWebGLSceneMetaData(vtkSMViewProxy* view)
{
  if (!view)
  {
    vtkErrorMacro("No view specified.");
    return nullptr;
  }

  vtkRenderWindow* renWin = view->GetRenderWindow();
  vtkPVRenderView* pvRenderView = vtkPVRenderView::SafeDownCast(view->GetClientSideView());

  if (renWin == nullptr || pvRenderView == nullptr)
  {
    vtkErrorMacro("The view is supported for WebGL export: " << view);
    return nullptr;
  }

  // We use the camera focal point to be the center of rotation
  double centerOfRotation[3];
  vtkCamera* cam = pvRenderView->GetActiveCamera();
  cam->GetFocalPoint(centerOfRotation);

  if (this->Internals->ViewWebGLMap.find(view) == this->Internals->ViewWebGLMap.end())
  {
    this->Internals->ViewWebGLMap[view] = vtkSmartPointer<vtkWebGLExporter>::New();
  }

  vtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  webglExporter->parseScene(renWin->GetRenderers(), view->GetGlobalIDAsString(), VTK_PARSEALL);

  vtkInternals::WebGLObjId2IndexMap webglMap;
  for (int i = 0; i < webglExporter->GetNumberOfObjects(); ++i)
  {
    vtkWebGLObject* wObj = webglExporter->GetWebGLObject(i);
    if (wObj && wObj->isVisible())
    {
      vtkInternals::WebGLObjCacheValue val;
      val.ObjIndex = i;
      for (int j = 0; j < wObj->GetNumberOfParts(); ++j)
      {
        val.BinaryParts[j] = "";
      }
      webglMap[wObj->GetId()] = val;
    }
  }
  this->Internals->WebGLExporterObjIdMap[webglExporter] = webglMap;
  webglExporter->SetCenterOfRotation(static_cast<float>(centerOfRotation[0]),
    static_cast<float>(centerOfRotation[1]), static_cast<float>(centerOfRotation[2]));
  return webglExporter->GenerateMetadata();
}

//----------------------------------------------------------------------------
const char* vtkPVWebApplication::GetWebGLBinaryData(vtkSMViewProxy* view, const char* id, int part)
{
  if (!view)
  {
    vtkErrorMacro("No view specified.");
    return nullptr;
  }
  if (this->Internals->ViewWebGLMap.find(view) == this->Internals->ViewWebGLMap.end())
  {
    if (this->GetWebGLSceneMetaData(view) == nullptr)
    {
      vtkErrorMacro("Failed to generate WebGL MetaData for: " << view);
      return nullptr;
    }
  }

  vtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  if (webglExporter == nullptr)
  {
    vtkErrorMacro("There is no cached WebGL Exporter for: " << view);
    return nullptr;
  }

  if (this->Internals->WebGLExporterObjIdMap[webglExporter].size() > 0 &&
    this->Internals->WebGLExporterObjIdMap[webglExporter].find(id) !=
      this->Internals->WebGLExporterObjIdMap[webglExporter].end())
  {
    vtkInternals::WebGLObjCacheValue* cachedVal =
      &(this->Internals->WebGLExporterObjIdMap[webglExporter][id]);
    if (cachedVal->BinaryParts.find(part) != cachedVal->BinaryParts.end())
    {
      if (cachedVal->BinaryParts[part].empty())
      {
        vtkWebGLObject* obj = webglExporter->GetWebGLObject(cachedVal->ObjIndex);
        if (obj && obj->isVisible())
        {
          // Manage Base64
          vtkNew<vtkBase64Utilities> base64;
          unsigned char* output = new unsigned char[obj->GetBinarySize(part) * 2];
          int size =
            base64->Encode(obj->GetBinaryData(part), obj->GetBinarySize(part), output, false);
          cachedVal->BinaryParts[part] = std::string((const char*)output, size);
          delete[] output;
        }
      }
      return cachedVal->BinaryParts[part].c_str();
    }
  }

  return nullptr;
}
//----------------------------------------------------------------------------
void vtkPVWebApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageEncoding: " << this->ImageEncoding << endl;
  os << indent << "ImageCompression: " << this->ImageCompression << endl;
}
