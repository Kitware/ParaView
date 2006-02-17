/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneProxy.h"

#include "vtkAnimationCue.h"
#include "vtkAnimationScene.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkErrorCode.h"
#include "vtkGenericMovieWriter.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkMPEG2Writer.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXMLPVAnimationWriterProxy.h"
#include "vtkTIFFWriter.h"
#include "vtkToolkits.h"

#ifdef _WIN32
  #include "vtkAVIWriter.h"
#else
#ifdef VTK_USE_FFMPEG_ENCODER
  #include "vtkFFMPEGWriter.h"
#endif
#endif

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkCxxRevisionMacro(vtkSMAnimationSceneProxy, "1.16.2.3");
vtkStandardNewMacro(vtkSMAnimationSceneProxy);

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies = vtkCollection::New();
  this->AnimationCueProxiesIterator = this->AnimationCueProxies->NewIterator();
  this->RenderModuleProxy = 0;
  this->GeometryCached = 0;
  
  this->MovieWriter = NULL;
  this->ImageWriter = NULL;
  this->FileRoot = NULL;
  this->FileExtension = NULL;

  this->GeometryWriter = 0;

  // Used to store the magnification of the saved image
  this->Magnification = 1;

  this->InSaveAnimation = 0;
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies->Delete();
  this->AnimationCueProxiesIterator->Delete();
  this->SetRenderModuleProxy(0);

  if (this->ImageWriter)
    {
    this->ImageWriter->Delete();
    this->ImageWriter = NULL;
    }
  if (this->MovieWriter)
    {
    this->MovieWriter->Delete();
    this->MovieWriter = NULL;
    }
  this->SetFileRoot(0);
  this->SetFileExtension(0);

  if (this->GeometryWriter)
    {
    this->GeometryWriter->Delete();
    this->GeometryWriter = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->AnimationCue = vtkAnimationScene::New();
  this->InitializeObservers(this->AnimationCue);
  this->ObjectsCreated = 1;

  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetCaching(int enable)
{
  this->Superclass::SetCaching(enable);
  vtkCollectionIterator* iter = this->AnimationCueProxies->NewIterator();
  
  for (iter->InitTraversal();
    !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMAnimationCueProxy* cue = 
      vtkSMAnimationCueProxy::SafeDownCast(iter->GetCurrentObject());
    cue->SetCaching(enable);
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SaveImages()
{
  if (!this->RenderModuleProxy)
    {
    return;
    }
  int errcode = 0;

  vtkImageData* capture = 
    this->RenderModuleProxy->CaptureWindow(this->Magnification);
  if (this->ImageWriter)
    {
    char* fileName = 
      new char[strlen(this->FileRoot) + strlen(this->FileExtension) + 25];
    sprintf(fileName, 
            "%s%04d.%s", this->FileRoot, this->FileCount, this->FileExtension);
    this->ImageWriter->SetInput(capture);
    this->ImageWriter->SetFileName(fileName);
    this->ImageWriter->Write();
    errcode = this->ImageWriter->GetErrorCode(); 
    this->FileCount = (!errcode)? this->FileCount + 1 : this->FileCount; 
    delete [] fileName;
    }
  else if (this->MovieWriter)
    {
    this->MovieWriter->SetInput(capture);
    this->MovieWriter->Write();

    int alg_error = this->MovieWriter->GetErrorCode();
    int movie_error = this->MovieWriter->GetError();

    if (movie_error && !alg_error)
      {
      //An error that the moviewriter caught, without setting any error code.
      //vtkGenericMovieWriter::GetStringFromErrorCode will result in
      //Unassigned Error. If this happens the Writer should be changed to set
      //a meaningful error code.

      errcode = vtkErrorCode::UserError;      
      }
    else
      {
      //if 0, then everything went well

      //< userError, means a vtkAlgorithm error (see vtkErrorCode.h)
      //= userError, means an unknown Error (Unassigned error)
      //> userError, means a vtkGenericMovieWriter error

      errcode = alg_error;
      }
    }
  if (errcode)
    {
    this->Stop();
    this->SaveFailed = errcode;
    }
  capture->Delete();
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::SaveImages(const char* fileRoot, 
                                         const char* ext, 
                                         int width, 
                                         int height, 
                                         double framerate,
                                         int quality)
{

  if (this->InSaveAnimation || 
      this->ImageWriter || this->MovieWriter || !this->RenderModuleProxy)
    {
    vtkErrorMacro("Incosistent state. Save aborted.");
    return 1;
    }
  this->InSaveAnimation = 1;
  this->SetAnimationTime(0);

  this->RenderModuleProxy->UpdateInformation();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("RenderWindowSizeInfo"));
  int *size = ivp->GetElements();

  this->Magnification = 1;
  // determine magnification.
  if (size[0] < width || size[1] < height)
    {
    int xMag = width / size[0] + 1;
    int yMag = height / size[1] + 1;
    this->Magnification = (xMag > yMag) ? xMag : yMag;
    width /= this->Magnification;
    height /= this->Magnification;
    }

  vtkSMIntVectorProperty* ivpSize = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderModuleProxy->GetProperty("RenderWindowSize"));
  ivpSize->SetElement(0, width);
  ivpSize->SetElement(1, height);
  this->RenderModuleProxy->UpdateVTKObjects();

  if (strcmp(ext,"jpg") == 0)
    {
    this->ImageWriter = vtkJPEGWriter::New();
    }
  else if (strcmp(ext,"tif") == 0)
    {
    this->ImageWriter = vtkTIFFWriter::New();
    }
  else if (strcmp(ext,"png") == 0)
    {
    this->ImageWriter = vtkPNGWriter::New();
    }
  else if (strcmp(ext, "mpg") == 0)
    {
    this->MovieWriter = vtkMPEG2Writer::New();
    }
#ifdef _WIN32
  else if (strcmp(ext, "avi") == 0)
    {
    vtkAVIWriter *aviwriter = vtkAVIWriter::New();
    aviwriter->SetQuality(quality);
    this->MovieWriter = aviwriter;
    }
#else
#ifdef VTK_USE_FFMPEG_ENCODER
  else if (strcmp(ext, "avi") == 0)
    {
    vtkFFMPEGWriter *aviwriter = vtkFFMPEGWriter::New();
    aviwriter->SetQuality(quality);
    this->MovieWriter = aviwriter;
    
    }
#endif
#endif
  else
    {
    vtkErrorMacro("Unknown extension " << ext << ", try: jpg, tif or png.");
    this->InSaveAnimation = 0;
    return 1;
    }

#ifndef VTK_USE_FFMPEG_ENCODER
  quality = quality + 1; //prevent unused parameter warning when no AVI
#endif

  this->SetFileRoot(fileRoot);
  this->SetFileExtension(ext);
  this->FileCount = 0;
  this->SaveFailed = 0;
  if (this->MovieWriter)
    {
    vtkImageData* capture = 
      this->RenderModuleProxy->CaptureWindow(this->Magnification);
    ostrstream str;
    str << fileRoot << "." << ext << ends;
    this->MovieWriter->SetFileName(str.str());
    str.rdbuf()->freeze(0);
    this->MovieWriter->SetInput(capture);
    this->MovieWriter->Start();
    capture->Delete();
    }

  // Play the animation.
  int oldMode = this->GetPlayMode();
  double old_framerate = this->GetFrameRate();
  int old_loop = this->GetLoop();
  this->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
  this->SetFrameRate(framerate);
  this->SetLoop(0);
  this->Play();
  this->SetPlayMode(oldMode);
  this->SetFrameRate(old_framerate);
  this->SetLoop(old_loop);

  if (this->ImageWriter)
    {
    if (this->SaveFailed)
      {
      char* fileName = new char[strlen(this->FileRoot) + strlen(this->FileExtension) + 25];
      for (int i=0; i < this->FileCount; i++)
        {
        sprintf(fileName, "%s%04d.%s", this->FileRoot, i, this->FileExtension);
        unlink(fileName);
        }
      delete [] fileName;
      }
    
    this->ImageWriter->Delete();
    this->ImageWriter = NULL;
    }
  else if (this->MovieWriter)
    {
    this->MovieWriter->End();

    if (this->SaveFailed)
      {
      char *fileName = this->MovieWriter->GetFileName();
      unlink(fileName);
      }

    this->MovieWriter->SetInput(0);
    this->MovieWriter->Delete();
    this->MovieWriter = NULL;
    }
  this->InSaveAnimation = 0;
  return this->SaveFailed;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SaveGeometry(double time)
{
  if (!this->GeometryWriter)
    {
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GeometryWriter->GetProperty("WriteTime"));
  dvp->SetElement(0, time);
  this->GeometryWriter->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::SaveGeometry(const char* filename)
{
  if (this->GeometryWriter || !this->RenderModuleProxy)
    {
    vtkErrorMacro("Inconsistent state! Cannot SaveGeometry");
    return 1;
    }
  vtkSMXMLPVAnimationWriterProxy* animWriter = 
    vtkSMXMLPVAnimationWriterProxy::SafeDownCast(vtkSMObject::GetProxyManager()
      ->NewProxy("writers","XMLPVAnimationWriter"));
  if (!animWriter)
    {
    vtkErrorMacro("Failed to create XMLPVAnimationWriter proxy.");
    return 1;
    }
  
  this->SaveFailed = 0;
  this->SetAnimationTime(0);
  this->GeometryWriter = animWriter;

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    animWriter->GetProperty("FileName"));
  svp->SetElement(0,filename);
  animWriter->UpdateVTKObjects();

  vtkSMProxyIterator* proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetMode(vtkSMProxyIterator::ONE_GROUP);
  proxyIter->Begin("displays");
  for (; !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    vtkSMDataObjectDisplayProxy* sDisp = vtkSMDataObjectDisplayProxy::SafeDownCast(
      proxyIter->GetProxy());
    // only the data object displays are saved.
    if (sDisp && sDisp->GetVisibilityCM())
      {
      sDisp->SetInputAsGeometryFilter(animWriter);
      }
    }
  proxyIter->Delete();

  vtkSMProperty* p = animWriter->GetProperty("Start");
  p->Modified();
  animWriter->UpdateVTKObjects();

  // Play the animation.
  int oldMode = this->GetPlayMode();
  int old_loop = this->GetLoop();
  this->SetLoop(0);
  this->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
  this->Play();
  this->SetPlayMode(oldMode);
  this->SetLoop(old_loop);
 
  p = animWriter->GetProperty("Finish");
  p->Modified();
  animWriter->UpdateVTKObjects();

  
  if (animWriter->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
    {
    this->SaveFailed = vtkErrorCode::OutOfDiskSpaceError;
    }
  animWriter->Delete();
  this->GeometryWriter = NULL;
  return this->SaveFailed;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SaveInBatchScript(ofstream* file)
{
  this->Superclass::SaveInBatchScript(file);
  vtkClientServerID id = this->SelfID;

  *file << "  [$pvTemp" << id << " GetProperty Loop]"
    << " SetElements1 " << this->GetLoop() << endl;
  *file << "  [$pvTemp" << id << " GetProperty FrameRate]"
    << " SetElements1 " << this->GetFrameRate() << endl;
  *file << "  [$pvTemp" << id << " GetProperty PlayMode]"
    << " SetElements1 " << this->GetPlayMode() << endl;
//TODO: How to set this?
  *file << "  $pvTemp" << id << " SetRenderModuleProxy $Ren1" << endl;
  *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
  *file << endl;
  vtkCollectionIterator* iter = this->AnimationCueProxiesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkSMAnimationCueProxy* cue = 
      vtkSMAnimationCueProxy::SafeDownCast(iter->GetCurrentObject());
    if (cue)
      {
      cue->SaveInBatchScript(file);
      *file << "  [$pvTemp" << id << " GetProperty Cues]"
        " AddProxy $pvTemp" << cue->GetID() << endl;
      *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
      *file << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::Play()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->Play();
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::IsInPlay()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->IsInPlay();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::Stop()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->Stop();
    }

}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetLoop(int loop)
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->SetLoop(loop);
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::GetLoop()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->GetLoop();
    }
  vtkErrorMacro("VTK object not created yet");
  return 0;
}
//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetFrameRate(double framerate)
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->SetFrameRate(framerate);
    }
}

//----------------------------------------------------------------------------
double vtkSMAnimationSceneProxy::GetFrameRate()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->GetFrameRate();
    }
  vtkErrorMacro("VTK object not created yet");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::AddCue(vtkSMProxy* proxy)
{
  vtkSMAnimationCueProxy* cue = vtkSMAnimationCueProxy::SafeDownCast(proxy);
  if (!cue)
    {
    return;
    }
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (!scene)
    {
    return;
    }
  if (this->AnimationCueProxies->IsItemPresent(cue))
    {
    vtkErrorMacro("Animation cue already present in the scene");
    return;
    }
  scene->AddCue(cue->GetAnimationCue());
  this->AnimationCueProxies->AddItem(cue);
  cue->SetCaching(this->GetCaching());
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveCue(vtkSMProxy* proxy)
{
  vtkSMAnimationCueProxy* smCue = vtkSMAnimationCueProxy::SafeDownCast(proxy);
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (!smCue || !scene)
    {
    return;
    }
  if (!this->AnimationCueProxies->IsItemPresent(smCue))
    {
    return;
    }
  scene->RemoveCue(smCue->GetAnimationCue());
  this->AnimationCueProxies->RemoveItem(smCue);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetPlayMode(int mode)
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->SetPlayMode(mode);
    }
  // Caching is disabled when play mode is real time.
  if (mode == vtkAnimationScene::PLAYMODE_REALTIME && this->Caching)
    {
    vtkWarningMacro("Disabling caching. "
      "Caching not available in Real Time mode.");
    this->SetCaching(0);
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::GetPlayMode()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->GetPlayMode();
    }
  vtkErrorMacro("VTK object was not created");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::StartCueInternal(void* info)
{
  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->StillRender();
    }
  this->Superclass::StartCueInternal(info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::TickInternal(void* info)
{
  this->CacheUpdate(info);
  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->StillRender();
    }
  this->Superclass::TickInternal(info);
  if (this->InSaveAnimation)
    {
    this->SaveImages();
    }
  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(info);
  this->SaveGeometry(cueInfo->AnimationTime);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::EndCueInternal(void* info)
{
  this->CacheUpdate(info);
  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->StillRender();
    }
  this->Superclass::EndCueInternal(info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CacheUpdate(void* info)
{
  if (!this->GetCaching() || 
      this->GetPlayMode() == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    return;
    }
  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(info);

  double etime = this->GetEndTime();
  double stime = this->GetStartTime();

  int index = 
    static_cast<int>((cueInfo->AnimationTime - stime) * this->GetFrameRate());

  int maxindex = 
    static_cast<int>((etime - stime) * this->GetFrameRate()) + 1; 

  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->CacheUpdate(index, maxindex);
    this->GeometryCached = 1;
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CleanCache()
{
  if (this->GeometryCached && this->RenderModuleProxy)
    {    
    this->RenderModuleProxy->InvalidateAllGeometries();
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetAnimationTime(double time)
{
  this->AnimationCue->Initialize();
  this->AnimationCue->Tick(time,0);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
