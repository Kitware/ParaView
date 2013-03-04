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
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPVDataEncoder.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVWebInteractionEvent.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGLExporter.h"
#include "vtkWebGLObject.h"

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
    ImageCacheValueType() : NeedsRender(true), HasImagesBeingProcessed(false) { }
    };
  typedef std::map<void*, ImageCacheValueType> ImageCacheType;
  ImageCacheType ImageCache;

  typedef std::map<void*, unsigned int > ButtonStatesType;
  ButtonStatesType ButtonStates;

  vtkNew<vtkPVDataEncoder> Encoder;

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
vtkPVWebApplication::vtkPVWebApplication():
  ImageEncoding(ENCODING_BASE64),
  ImageCompression(COMPRESSION_JPEG),
  Internals(new vtkPVWebApplication::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkPVWebApplication::~vtkPVWebApplication()
{
  delete this->Internals;
  this->Internals = NULL;
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
    return NULL;
    }

  vtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];

  if (value.NeedsRender == false &&
    value.Data != NULL &&
    view->HasDirtyRepresentation() == false)
    {
    //cout <<  "Reusing cache" << endl;
    bool latest = this->Internals->Encoder->GetLatestOutput(view->GetGlobalID(), value.Data);
    value.HasImagesBeingProcessed = !latest;
    return value.Data;
    }

  //cout <<  "Regenerating " << endl;
  //vtkTimerLog::ResetLog();
  //vtkTimerLog::CleanupLog();
  //vtkTimerLog::MarkStartEvent("StillRenderToString");
  //vtkTimerLog::MarkStartEvent("CaptureWindow");

  // TODO: We should add logic to check if a new rendering needs to be done and
  // then alone do a new rendering otherwise use the cached image.
  vtkImageData* image = view->CaptureWindow(1);
  //vtkTimerLog::MarkEndEvent("CaptureWindow");

  //vtkTimerLog::MarkEndEvent("StillRenderToString");
  //vtkTimerLog::DumpLogWithIndents(&cout, 0.0);

  this->Internals->Encoder->PushAndTakeReference(view->GetGlobalID(), image, quality);
  assert(image == NULL);

  if (value.Data == NULL)
    {
    // we need to wait till output is processed.
    //cout << "Flushing" << endl;
    this->Internals->Encoder->Flush(view->GetGlobalID());
    //cout << "Done Flushing" << endl;
    }

  bool latest = this->Internals->Encoder->GetLatestOutput(view->GetGlobalID(), value.Data);
  value.HasImagesBeingProcessed = !latest;
  value.NeedsRender = false;
  return value.Data;
}

//----------------------------------------------------------------------------
const char* vtkPVWebApplication::StillRenderToString(vtkSMViewProxy* view, unsigned long time, int quality)
{
  vtkUnsignedCharArray* array = this->StillRender(view, quality);
  if (array && array->GetMTime() != time)
    {
    this->LastStillRenderToStringMTime = array->GetMTime();
    cout << "Image size: " << array->GetNumberOfTuples() << endl;
    return reinterpret_cast<char*>(array->GetPointer(0));
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkPVWebApplication::HandleInteractionEvent(
  vtkSMViewProxy* view, vtkPVWebInteractionEvent* event)
{
  vtkSMRenderViewProxy* rvview = vtkSMRenderViewProxy::SafeDownCast(view);
  vtkSMContextViewProxy* ctxView = vtkSMContextViewProxy::SafeDownCast(view);

  vtkRenderWindow* renWin = NULL;
  vtkRenderWindowInteractor *iren = NULL;

  if (rvview)
    {
    renWin = rvview->GetRenderWindow();
    iren = rvview->GetInteractor();
    }
  else if (ctxView)
    {
    renWin = ctxView->GetRenderWindow();
    iren = renWin->GetInteractor();
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

  int ctrlKey =
    (event->GetModifiers() & vtkPVWebInteractionEvent::CTRL_KEY) != 0?  1: 0;
  int shiftKey =
    (event->GetModifiers() & vtkPVWebInteractionEvent::SHIFT_KEY) != 0?  1: 0;
  iren->SetEventInformation(posX, posY, ctrlKey, shiftKey, event->GetKeyCode());


  unsigned int prev_buttons = this->Internals->ButtonStates[view];
  unsigned int changed_buttons = (event->GetButtons() ^ prev_buttons);
  iren->MouseMoveEvent();
  if ( (changed_buttons & vtkPVWebInteractionEvent::LEFT_BUTTON) != 0 )
    {
    if ( (event->GetButtons() & vtkPVWebInteractionEvent::LEFT_BUTTON) != 0)
      {
      iren->LeftButtonPressEvent();
      }
    else
      {
      iren->LeftButtonReleaseEvent();
      }
    }

  if ( (changed_buttons & vtkPVWebInteractionEvent::RIGHT_BUTTON) != 0 )
    {
    if ( (event->GetButtons() & vtkPVWebInteractionEvent::RIGHT_BUTTON) != 0)
      {
      iren->RightButtonPressEvent();
      }
    else
      {
      iren->RightButtonReleaseEvent();
      }
    }
  if ( (changed_buttons & vtkPVWebInteractionEvent::MIDDLE_BUTTON) != 0 )
    {
    if ( (event->GetButtons() & vtkPVWebInteractionEvent::MIDDLE_BUTTON) != 0)
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

//----------------------------------------------------------------------------
const char* vtkPVWebApplication::GetWebGLBinaryData(
  vtkSMViewProxy* view, const char* id, int part)
{
  if (!view)
    {
    vtkErrorMacro("No view specified.");
    return NULL;
    }
  if(this->Internals->ViewWebGLMap.find(view) ==
    this->Internals->ViewWebGLMap.end())
    {
    if(this->GetWebGLSceneMetaData(view) == NULL)
      {
      vtkErrorMacro("Failed to generate WebGL MetaData for: " << view);
      return NULL;
      }
    }

  vtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  if(webglExporter == NULL)
    {
    vtkErrorMacro("There is no cached WebGL Exporter for: " << view);
    return NULL;
    }

  if(this->Internals->WebGLExporterObjIdMap[webglExporter].size() > 0 &&
    this->Internals->WebGLExporterObjIdMap[webglExporter].find(id) !=
    this->Internals->WebGLExporterObjIdMap[webglExporter].end())
    {
    vtkInternals::WebGLObjCacheValue* cachedVal =
      &(this->Internals->WebGLExporterObjIdMap[webglExporter][id]);
    if(cachedVal->BinaryParts.find(part) != cachedVal->BinaryParts.end())
      {
      if(cachedVal->BinaryParts[part].empty())
        {
        vtkWebGLObject* obj = webglExporter->GetObject(cachedVal->ObjIndex);
        if(obj && obj->isVisible())
          {
          // Manage Base64
          vtkNew<vtkBase64Utilities> base64;
          unsigned char* output = new unsigned char[obj->GetBinarySize(part)*2];
          int size = base64->Encode(
            obj->GetBinaryData(part), obj->GetBinarySize(part), output, false);
          cachedVal->BinaryParts[part] = std::string((const char *)output, size);
          delete[] output;
          }
        }
      return cachedVal->BinaryParts[part].c_str();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVWebApplication::GetWebGLBinaryObjects(vtkSMViewProxy* view)
{
  if (!view)
    {
    vtkErrorMacro("No view specified.");
    return NULL;
    }
  vtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  if(webglExporter == NULL)
    {
    vtkErrorMacro("There is no cached WebGL Exporter for: " << view);
    return NULL;
    }

  this->Internals->LastAllWebGLBinaryObjects = "[";
  std::string strObj;
  for(int i=0; i<webglExporter->GetNumberOfObjects(); ++i)
    {
    vtkWebGLObject* wObj = webglExporter->GetObject(i);
    if(wObj && wObj->isVisible())
      {
      for(int j=0; j<wObj->GetNumberOfParts(); ++j)
        {
        strObj = this->GetWebGLBinaryData(view, wObj->GetId().c_str(), j);
        this->Internals->LastAllWebGLBinaryObjects += "'" + strObj + "',\n";
        }
      }
    }
  this->Internals->LastAllWebGLBinaryObjects += "'']";
  return this->Internals->LastAllWebGLBinaryObjects.c_str();
}

//----------------------------------------------------------------------------
void vtkPVWebApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageEncoding: " << this->ImageEncoding << endl;
  os << indent << "ImageCompression: " << this->ImageCompression << endl;
}
