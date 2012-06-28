/*=========================================================================

  Program:   ParaView
  Module:    vtkPVView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVView.h"

#include "vtkCacheSizeKeeper.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVSession.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkTimerLog.h"

#include <assert.h>
#include <map>


class vtkPVView::vtkInternals
{
private:
  typedef std::map<vtkPVSession*,
    vtkWeakPointer<vtkPVSynchronizedRenderWindows> > MapOfSynchronizedWindows;
  static MapOfSynchronizedWindows SynchronizedWindows;
public:
  static vtkPVSynchronizedRenderWindows* NewSynchronizedWindows(
    vtkPVSession* session)
    {
    vtkPVSynchronizedRenderWindows* srw =
      vtkInternals::SynchronizedWindows[session].GetPointer();
    if (srw == NULL)
      {
      srw = vtkPVSynchronizedRenderWindows::New(session);
      vtkInternals::SynchronizedWindows[session] = srw;
      return srw;
      }
    else
      {
      srw->Register(NULL);
      return srw;
      }
    }
};

vtkPVView::vtkInternals::MapOfSynchronizedWindows
vtkPVView::vtkInternals::SynchronizedWindows;

vtkInformationKeyMacro(vtkPVView, REQUEST_RENDER, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE_LOD, Request);
vtkInformationKeyMacro(vtkPVView, REQUEST_UPDATE, Request);
vtkInformationKeyRestrictedMacro(vtkPVView, VIEW, ObjectBase, "vtkPVView");

bool vtkPVView::EnableStreaming = false; 
//----------------------------------------------------------------------------
void vtkPVView::SetEnableStreaming(bool val)
{
  vtkPVView::EnableStreaming = val;
}

//----------------------------------------------------------------------------
bool vtkPVView::GetEnableStreaming()
{
  return vtkPVView::EnableStreaming;
}

//----------------------------------------------------------------------------
vtkPVView::vtkPVView()
{
  // Ensure vtkProcessModule is setup correctly.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("vtkProcessModule not initialized. Aborting.");
    abort();
    }

  vtkPVSession* activeSession = vtkPVSession::SafeDownCast(pm->GetActiveSession());
  if (!activeSession)
    {
    vtkErrorMacro("Could not find any active session. Aborting.");
    abort();
    }

  this->SynchronizedWindows =
    vtkInternals::NewSynchronizedWindows(activeSession);
  this->Identifier = 0;
  this->ViewTime = 0.0;
  this->CacheKey = 0.0;
  this->UseCache = false;

  this->RequestInformation = vtkInformation::New();
  this->ReplyInformationVector = vtkInformationVector::New();

  this->ViewTimeValid = false;
  this->LastRenderOneViewAtATime = false;

  this->Size[1] = this->Size[0] = 300;
  this->Position[0] = this->Position[1] = 0;
}

//----------------------------------------------------------------------------
vtkPVView::~vtkPVView()
{
  this->SynchronizedWindows->RemoveAllRenderers(this->Identifier);
  this->SynchronizedWindows->RemoveRenderWindow(this->Identifier);
  this->SynchronizedWindows->Delete();
  this->SynchronizedWindows = NULL;

  this->RequestInformation->Delete();
  this->ReplyInformationVector->Delete();
}

//----------------------------------------------------------------------------
void vtkPVView::Initialize(unsigned int id)
{
  if (this->Identifier == id)
    {
    // already initialized
    return;
    }
  assert(this->Identifier == 0 && id != 0);

  this->Identifier = id;
  this->SetSize(this->Size[0], this->Size[1]);
  this->SetPosition(this->Position[0], this->Position[1]);
}

//----------------------------------------------------------------------------
void vtkPVView::SetPosition(int x, int y)
{
  if (this->Identifier != 0)
    {
    this->SynchronizedWindows->SetWindowPosition(this->Identifier, x, y);
    }
  this->Position[0] = x;
  this->Position[1] = y;
}

//----------------------------------------------------------------------------
void vtkPVView::SetSize(int x, int y)
{
  if (this->Identifier != 0)
    {
    this->SynchronizedWindows->SetWindowSize(this->Identifier, x, y);
    }
  this->Size[0] = x;
  this->Size[1] = y;
}

//----------------------------------------------------------------------------
void vtkPVView::SetViewTime(double time)
{
  if (this->ViewTime != time)
    {
    this->ViewTime = time;
    this->ViewTimeValid = true;
    this->InvokeEvent(ViewTimeChangedEvent);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkPVView::InTileDisplayMode()
{
  int temp[2];
  return this->SynchronizedWindows->GetTileDisplayParameters(temp, temp);
}

//----------------------------------------------------------------------------
bool vtkPVView::SynchronizeBounds(double bounds[6])
{
  return this->SynchronizedWindows->SynchronizeBounds(bounds);
}

//----------------------------------------------------------------------------
bool vtkPVView::SynchronizeSize(double &size)
{
  return this->SynchronizedWindows->SynchronizeSize(size);
}

//----------------------------------------------------------------------------
bool vtkPVView::SynchronizeSize(unsigned int &size)
{
  return this->SynchronizedWindows->SynchronizeSize(size);
}

//----------------------------------------------------------------------------
void vtkPVView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Identifier: " << this->Identifier << endl;
  os << indent << "ViewTime: " << this->ViewTime << endl;
  os << indent << "CacheKey: " << this->CacheKey << endl;
  os << indent << "UseCache: " << this->UseCache << endl;
}

//----------------------------------------------------------------------------
void vtkPVView::PrepareForScreenshot()
{
  if (!this->InTileDisplayMode())
    {
    this->LastRenderOneViewAtATime =
      this->SynchronizedWindows->GetRenderOneViewAtATime();
    this->SynchronizedWindows->RenderOneViewAtATimeOn();
    }
}

//----------------------------------------------------------------------------
void vtkPVView::CleanupAfterScreenshot()
{
  if (!this->InTileDisplayMode())
    {
    this->SynchronizedWindows->RenderOneViewAtATimeOff();
    this->SynchronizedWindows->SetRenderOneViewAtATime(
      this->LastRenderOneViewAtATime);
    }
}

//----------------------------------------------------------------------------
void vtkPVView::Update()
{
  vtkTimerLog::MarkStartEvent("vtkPVView::Update");
  // Ensure that cache size if synchronized among the processes.
  if (this->GetUseCache())
    {
    vtkCacheSizeKeeper* cacheSizeKeeper = vtkCacheSizeKeeper::GetInstance();
    unsigned int cache_full = 0;
    if (cacheSizeKeeper->GetCacheSize() > cacheSizeKeeper->GetCacheLimit())
      {
      cache_full = 1;
      }
    this->SynchronizedWindows->SynchronizeSize(cache_full);
    cacheSizeKeeper->SetCacheFull(cache_full > 0);
    }

  this->CallProcessViewRequest(vtkPVView::REQUEST_UPDATE(),
    this->RequestInformation, this->ReplyInformationVector);
  vtkTimerLog::MarkEndEvent("vtkPVView::Update");
}

//----------------------------------------------------------------------------
void vtkPVView::CallProcessViewRequest(
  vtkInformationRequestKey* type, vtkInformation* inInfo, vtkInformationVector* outVec)
{
  int num_reprs = this->GetNumberOfRepresentations();
  outVec->SetNumberOfInformationObjects(num_reprs);

  if (type == REQUEST_UPDATE())
    {
    // Pass the view time before updating the representations.
    for (int cc=0; cc < num_reprs; cc++)
      {
      vtkDataRepresentation* repr = this->GetRepresentation(cc);
      vtkPVDataRepresentation* pvrepr = vtkPVDataRepresentation::SafeDownCast(repr);
      if (pvrepr)
        {
        // Pass the view time information to the representation
        if(this->ViewTimeValid)
          {
          pvrepr->SetUpdateTime(this->GetViewTime());
          }

        pvrepr->SetUseCache(this->GetUseCache());
        pvrepr->SetCacheKey(this->GetCacheKey());
        }
      }
    }

  // NOTE: This will create a reference loop (depending on what inInfo is). If
  // it's this->RequestInformation, then we have a loop and hence it's
  // essential to call vtkInformation::Clear() before this method returns.
  inInfo->Set(VIEW(), this);

  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkInformation* outInfo = outVec->GetInformationObject(cc);
    outInfo->Clear();
    vtkDataRepresentation* repr = this->GetRepresentation(cc);
    vtkPVDataRepresentation* pvrepr = vtkPVDataRepresentation::SafeDownCast(repr);
    if (pvrepr)
      {
      pvrepr->ProcessViewRequest(type, inInfo, outInfo);
      }
    else if (repr && type == REQUEST_UPDATE())
      {
      repr->Update();
      }
    }

  // Clear input information since we are done with the pass. This avoids any
  // need for garbage collection.
  inInfo->Clear();
}
