/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeView.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkImageData.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkProcessModule.h"
#include "vtkSMCameraLink.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVectorOperators.h"

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <assert.h>

namespace
{
typedef vtkTypeInt64 Fixed64;
static inline Fixed64 toFixed(int i)
{
  return (Fixed64)i * 256;
}
static inline int fRound(Fixed64 i)
{
  return (i % 256 < 128) ? i / 256 : 1 + i / 256;
}
struct LayoutStruct
{
  int pos;
  int size;
};

// This code is loosely based on the QGridLayout's workhouse qGeomCalc (in qlayoutengine.cpp
// for Qt version 5.6) which portions out available space to a collection of items in a row or
// a column.
//
// The calculation is done in fixed point: "fixed" variables are scaled by a factor of 256.
void vtkGeomCalc(
  std::vector<LayoutStruct>& chain, int start, int count, int pos, int space, int spacing)
{
  assert(start == 0 && count > 1 && pos >= 0 && space >= 0 && space >= 0);
  // amount of space left after borders are added between views.
  int space_left = space - (count > 1 ? count - 1 : 0) * spacing;

  int space_allocated = 0;
  Fixed64 fp_space = toFixed(space_left);
  Fixed64 fp_w = 0;
  for (int i = start; i < count; ++i)
  {
    LayoutStruct* data = &chain[i];
    fp_w += (fp_space / count);
    int w = fRound(fp_w);
    data->size = w;
    fp_w -= toFixed(w); // give the difference to the next.
    space_allocated += w;
  }
  space_left -= space_allocated;
  assert(space_left == 0);

  chain[start].pos = pos;
  for (int i = start + 1; i < count; ++i)
  {
    chain[i].pos = chain[i - 1].pos + chain[i - 1].size + spacing;
  }
}
}

#define ENSURE_INIT()                                                                              \
  if (this->RootView == NULL)                                                                      \
  {                                                                                                \
    return;                                                                                        \
  }

//----------------------------------------------------------------------------
namespace
{
static void vtkCopyClone(
  vtkSMProxy* source, vtkSMProxy* clone, std::set<std::string>* exceptions = 0)
{
  vtkSmartPointer<vtkSMPropertyIterator> iterSource;
  vtkSmartPointer<vtkSMPropertyIterator> iterDest;

  iterSource.TakeReference(source->NewPropertyIterator());
  iterDest.TakeReference(clone->NewPropertyIterator());

  // Since source/clone are exact copies, the following is safe.
  for (; !iterSource->IsAtEnd() && !iterDest->IsAtEnd(); iterSource->Next(), iterDest->Next())
  {
    // Skip the property if it is in the exceptions list.
    if (exceptions && exceptions->find(iterSource->GetKey()) != exceptions->end())
    {
      continue;
    }
    vtkSMProperty* destProp = iterDest->GetProperty();

    // Skip the property if it is information only or is internal.
    if (destProp->GetInformationOnly())
    {
      continue;
    }

    // Copy the property from source to dest
    destProp->Copy(iterSource->GetProperty());
  }
}

static void vtkAddRepresentation(vtkSMProxy* view, vtkSMProxy* repr)
{
  // We are always using cache in this view.
  vtkSMPropertyHelper(repr, "ForceUseCache", true).Set(1);
  vtkSMPropertyHelper(repr, "ForcedCacheKey", true).Set(1);
  repr->UpdateVTKObjects();

  vtkSMPropertyHelper(view, "Representations").Add(repr);
  view->UpdateVTKObjects();
}

static void vtkRemoveRepresentation(vtkSMProxy* view, vtkSMProxy* repr)
{
  vtkSMPropertyHelper(view, "Representations").Remove(repr);
  view->UpdateVTKObjects();
}
}

//----------------------------------------------------------------------------
class vtkPVComparativeView::vtkInternal
{
public:
  struct RepresentationCloneItem
  {
    // The clone representation proxy.
    vtkSmartPointer<vtkSMProxy> CloneRepresentation;

    // The sub-view in which this clone exists.
    vtkSmartPointer<vtkSMViewProxy> ViewProxy;

    RepresentationCloneItem() {}
    RepresentationCloneItem(vtkSMViewProxy* view, vtkSMProxy* repr)
      : CloneRepresentation(repr)
      , ViewProxy(view)
    {
    }
  };

  struct RepresentationData
  {
    typedef std::vector<RepresentationCloneItem> VectorOfClones;
    VectorOfClones Clones;
    vtkSmartPointer<vtkSMProxyLink> Link;

    // Returns the representation clone in the given view.
    VectorOfClones::iterator FindRepresentationClone(vtkSMViewProxy* view)
    {
      VectorOfClones::iterator iter;
      for (iter = this->Clones.begin(); iter != this->Clones.end(); ++iter)
      {
        if (iter->ViewProxy == view)
        {
          return iter;
        }
      }
      return this->Clones.end();
    }
  };

  typedef std::vector<vtkSmartPointer<vtkSMViewProxy> > VectorOfViews;
  VectorOfViews Views;

  typedef std::map<vtkSMProxy*, RepresentationData> MapOfReprClones;
  MapOfReprClones RepresentationClones;

  typedef std::vector<vtkSmartPointer<vtkSMComparativeAnimationCueProxy> > VectorOfCues;
  VectorOfCues Cues;

  vtkSmartPointer<vtkSMProxyLink> ViewLink;
  vtkSmartPointer<vtkSMCameraLink> ViewCameraLink;

  vtkInternal()
  {
    this->ViewLink = vtkSmartPointer<vtkSMProxyLink>::New();
    this->ViewCameraLink = vtkSmartPointer<vtkSMCameraLink>::New();
    this->ViewCameraLink->SynchronizeInteractiveRendersOff();
  }

  // Creates a new representation clone and adds it in the given view.
  // Arguments:
  // @repr -- representation to clone
  // @view -- the view to which the new clone should be added.
  vtkSMProxy* AddRepresentationClone(vtkSMProxy* repr, vtkSMViewProxy* view)
  {
    MapOfReprClones::iterator iter = this->RepresentationClones.find(repr);
    if (iter == this->RepresentationClones.end())
    {
      vtkGenericWarningMacro("This representation cannot be cloned!!!");
      return NULL;
    }

    vtkInternal::RepresentationData& data = iter->second;

    vtkSMSessionProxyManager* pxm = repr->GetSessionProxyManager();

    // Create a new representation
    vtkSMProxy* newRepr = pxm->NewProxy(repr->GetXMLGroup(), repr->GetXMLName());

    // Made the new representation a clone
    vtkCopyClone(repr, newRepr);
    newRepr->UpdateVTKObjects();
    data.Link->AddLinkedProxy(newRepr, vtkSMLink::OUTPUT);

    // Add the cloned representation to the view
    vtkAddRepresentation(view, newRepr);

    // Add the cloned representation to the RepresentationData struct
    // The clone is added to a map where its view is the key.
    data.Clones.push_back(vtkInternal::RepresentationCloneItem(view, newRepr));

    // Clean up this reference
    newRepr->Delete();
    return newRepr;
  }

  unsigned int ActiveIndexX;
  unsigned int ActiveIndexY;
  std::string SuggestedViewType;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVComparativeView);
vtkCxxSetObjectMacro(vtkPVComparativeView, RootView, vtkSMViewProxy);
//----------------------------------------------------------------------------
vtkPVComparativeView::vtkPVComparativeView()
{
  this->Internal = new vtkInternal();
  this->RootView = NULL;
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->ViewSize[0] = 400;
  this->ViewSize[1] = 400;
  this->ViewPosition[0] = 0;
  this->ViewPosition[1] = 0;
  this->ViewTime = 0.0;
  this->OverlayAllComparisons = false;
  this->Spacing[0] = this->Spacing[1] = 1;

  this->Outdated = true;

  vtkMemberFunctionCommand<vtkPVComparativeView>* fsO =
    vtkMemberFunctionCommand<vtkPVComparativeView>::New();
  fsO->SetCallback(*this, &vtkPVComparativeView::MarkOutdated);
  this->MarkOutdatedObserver = fsO;
}

//----------------------------------------------------------------------------
vtkPVComparativeView::~vtkPVComparativeView()
{
  this->SetRootView(NULL);
  delete this->Internal;
  this->MarkOutdatedObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::AddCue(vtkSMComparativeAnimationCueProxy* cue)
{
  this->Internal->Cues.push_back(cue);
  cue->UpdateVTKObjects();
  vtkObject::SafeDownCast(cue->GetClientSideObject())
    ->AddObserver(vtkCommand::ModifiedEvent, this->MarkOutdatedObserver);
  this->MarkOutdated();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::RemoveCue(vtkSMComparativeAnimationCueProxy* cue)
{
  vtkInternal::VectorOfCues::iterator iter;
  for (iter = this->Internal->Cues.begin(); iter != this->Internal->Cues.end(); ++iter)
  {
    if (iter->GetPointer() == cue)
    {
      vtkObject::SafeDownCast(cue->GetClientSideObject())
        ->RemoveObserver(this->MarkOutdatedObserver);
      this->Internal->Cues.erase(iter);
      this->MarkOutdated();
      break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::Initialize(vtkSMViewProxy* rootView)
{
  if (this->RootView == rootView || !rootView)
  {
    return;
  }

  if (this->RootView)
  {
    vtkErrorMacro("vtkPVComparativeView::Initialize() can only be called once.");
    return;
  }

  this->SetRootView(rootView);
  ENSURE_INIT();

  // Root view is the first view in the views list.
  this->Internal->Views.push_back(rootView);

  this->Internal->ViewCameraLink->AddLinkedProxy(rootView, vtkSMLink::INPUT);
  this->Internal->ViewCameraLink->AddLinkedProxy(rootView, vtkSMLink::OUTPUT);
  this->Internal->ViewLink->AddLinkedProxy(rootView, vtkSMLink::INPUT);

  // Every view keeps their own representations.
  this->Internal->ViewLink->AddException("Representations");

  // This view computes view size/view position for each view based on the
  // layout.
  this->Internal->ViewLink->AddException("ViewSize");
  this->Internal->ViewLink->AddException("ViewTime");
  this->Internal->ViewLink->AddException("CacheKey");
  this->Internal->ViewLink->AddException("UseCache");
  this->Internal->ViewLink->AddException("ViewPosition");

  this->Internal->ViewLink->AddException("CameraPositionInfo");
  this->Internal->ViewLink->AddException("CameraPosition");
  this->Internal->ViewLink->AddException("CameraFocalPointInfo");
  this->Internal->ViewLink->AddException("CameraFocalPoint");
  this->Internal->ViewLink->AddException("CameraViewUpInfo");
  this->Internal->ViewLink->AddException("CameraViewUp");
  this->Internal->ViewLink->AddException("CameraViewAngleInfo");
  this->Internal->ViewLink->AddException("CameraViewAngle");

  this->Build(this->Dimensions[0], this->Dimensions[1]);
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::SetOverlayAllComparisons(bool overlay)
{
  if (this->OverlayAllComparisons == overlay)
  {
    return;
  }

  this->OverlayAllComparisons = overlay;
  this->Modified();

  this->Build(this->Dimensions[0], this->Dimensions[1]);
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::Build(int dx, int dy)
{
  if (dx <= 0 || dy <= 0)
  {
    vtkErrorMacro("Dimensions cannot be 0.");
    return;
  }

  this->Dimensions[0] = dx;
  this->Dimensions[1] = dy;

  ENSURE_INIT();

  size_t numViews = this->OverlayAllComparisons ? 1 : dx * dy;
  size_t cc;

  assert(numViews >= 1);

  // Remove extra view modules.
  for (cc = this->Internal->Views.size() - 1; cc >= numViews; cc--)
  {
    this->RemoveView(this->Internal->Views[cc]);
    this->Outdated = true;
  }

  // Add view modules, if not enough.
  for (cc = this->Internal->Views.size(); cc < numViews; cc++)
  {
    this->AddNewView();
    this->Outdated = true;
  }

  if (this->OverlayAllComparisons)
  {
    // ensure that there are enough representation clones in the root view to
    // match the dimensions.

    vtkSMViewProxy* root_view = this->GetRootView();
    size_t numReprs = dx * dy;
    vtkInternal::MapOfReprClones::iterator reprIter;
    for (reprIter = this->Internal->RepresentationClones.begin();
         reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
    {
      vtkSMProxy* repr = reprIter->first;
      vtkInternal::RepresentationData& data = reprIter->second;

      // remove old root-clones if extra.
      if (data.Clones.size() > (numReprs - 1))
      {
        for (cc = data.Clones.size() - 1; cc >= numReprs; cc--)
        {
          vtkSMProxy* root_clone = data.Clones[cc].CloneRepresentation;
          vtkRemoveRepresentation(root_view, root_clone);
          data.Link->RemoveLinkedProxy(root_clone);
        }
        data.Clones.resize(numReprs);
      }
      else
      {
        // add new root-clones if needed.
        for (cc = data.Clones.size(); cc < numReprs - 1; cc++)
        {
          this->Internal->AddRepresentationClone(repr, root_view);
        }
      }
    }
  }

  this->UpdateViewLayout();

  // Whenever the layout changes we'll fire the ConfigureEvent.
  this->InvokeEvent(vtkCommand::ConfigureEvent);
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::UpdateViewLayout()
{
  ENSURE_INIT();

  int numCols = this->OverlayAllComparisons ? 1 : this->Dimensions[0];
  int numRows = this->OverlayAllComparisons ? 1 : this->Dimensions[1];
  assert(numCols >= 1 && numRows >= 1);
  std::vector<LayoutStruct> colData(numCols);
  std::vector<LayoutStruct> rowData(numRows);

  // We mimic the logic from qlayoutengine.cpp (qGeomCalc) for sizing
  // cells in a QGridLayout.
  vtkGeomCalc(colData, 0, numCols, this->ViewPosition[0], this->ViewSize[0], this->Spacing[0]);
  vtkGeomCalc(rowData, 0, numRows, this->ViewPosition[1], this->ViewSize[1], this->Spacing[1]);

  for (int row = 0, view_index = 0; row < numRows; ++row)
  {
    for (int col = 0; col < numCols; ++col, ++view_index)
    {
      vtkSMViewProxy* view = this->Internal->Views[view_index];

      const vtkVector2i pos(colData[col].pos, rowData[row].pos);
      vtkSMPropertyHelper(view, "ViewPosition").Set(pos.GetData(), 2);

      const vtkVector2i size(colData[col].size, rowData[row].size);
      // Not all view classes have a ViewSize property
      vtkSMPropertyHelper(view, "ViewSize", true).Set(size.GetData(), 2);
      view->UpdateVTKObjects();
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::AddNewView()
{
  ENSURE_INIT();

  vtkSMViewProxy* rootView = this->GetRootView();
  vtkSMSessionProxyManager* pxm = rootView->GetSessionProxyManager();
  vtkSMViewProxy* newView =
    vtkSMViewProxy::SafeDownCast(pxm->NewProxy(rootView->GetXMLGroup(), rootView->GetXMLName()));
  if (!newView)
  {
    vtkErrorMacro("Failed to create internal view proxy. Comparative visualization "
                  "view cannot work.");
    return;
  }

  newView->UpdateVTKObjects();

  // Copy current view properties over to this newly created view.
  std::set<std::string> exceptions;
  exceptions.insert("Representations");
  exceptions.insert("ViewSize");
  exceptions.insert("UseCache");
  exceptions.insert("CacheKey");
  exceptions.insert("ViewPosition");
  vtkCopyClone(rootView, newView, &exceptions);

  this->Internal->Views.push_back(newView);
  this->Internal->ViewCameraLink->AddLinkedProxy(newView, vtkSMLink::INPUT);
  this->Internal->ViewCameraLink->AddLinkedProxy(newView, vtkSMLink::OUTPUT);
  this->Internal->ViewLink->AddLinkedProxy(newView, vtkSMLink::OUTPUT);
  newView->Delete();

  // Create clones for all currently added representation for the new view.
  vtkInternal::MapOfReprClones::iterator reprIter;
  for (reprIter = this->Internal->RepresentationClones.begin();
       reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
  {
    vtkSMProxy* repr = reprIter->first;

    vtkSMProxy* clone = this->Internal->AddRepresentationClone(repr, newView);
    static_cast<void>(clone); // unused variable warning.
    assert(clone != NULL);
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::RemoveView(vtkSMViewProxy* view)
{
  ENSURE_INIT();
  if (view == this->GetRootView())
  {
    vtkErrorMacro("Root view cannot be removed.");
    return;
  }

  // Remove all representations in this view.
  vtkInternal::MapOfReprClones::iterator reprIter;
  for (reprIter = this->Internal->RepresentationClones.begin();
       reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
  {
    vtkInternal::RepresentationData& data = reprIter->second;
    vtkInternal::RepresentationData::VectorOfClones::iterator cloneIter =
      data.FindRepresentationClone(view);
    if (cloneIter != data.Clones.end())
    {
      vtkSMProxy* clone = cloneIter->CloneRepresentation;
      vtkRemoveRepresentation(view, clone);
      data.Link->RemoveLinkedProxy(clone);
      data.Clones.erase(cloneIter);
    }
  }

  this->Internal->ViewLink->RemoveLinkedProxy(view);
  this->Internal->ViewCameraLink->RemoveLinkedProxy(view);
  this->Internal->ViewCameraLink->RemoveLinkedProxy(view);

  vtkInternal::VectorOfViews::iterator iter;
  for (iter = this->Internal->Views.begin(); iter != this->Internal->Views.end(); ++iter)
  {
    if (iter->GetPointer() == view)
    {
      this->Internal->Views.erase(iter);
      break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::AddRepresentation(vtkSMProxy* repr)
{
  ENSURE_INIT();

  if (!repr)
  {
    return;
  }

  this->MarkOutdated();

  // Add representation to the root view
  vtkSMViewProxy* rootView = this->GetRootView();
  vtkAddRepresentation(rootView, repr);

  // Create clones of representation and add them to all other views.
  // We will save information about the clones we create
  // so that we can clean them up later.
  vtkInternal::RepresentationData data;

  // We'll link the clones' properties to the original
  // representation using a proxy link.  The "UpdateTime"
  // property will not be linked however.
  vtkSMProxyLink* reprLink = vtkSMProxyLink::New();
  data.Link.TakeReference(reprLink);
  reprLink->AddLinkedProxy(repr, vtkSMLink::INPUT);
  reprLink->AddException("ForceUseCache");
  reprLink->AddException("ForcedCacheKey");

  // Add the RepresentationData struct to a map
  // with the original representation as the key
  this->Internal->RepresentationClones[repr] = data;

  // Now, for all existing sub-views, create representation clones.
  vtkInternal::VectorOfViews::iterator iter = this->Internal->Views.begin();
  iter++; // skip root view.
  for (; iter != this->Internal->Views.end(); ++iter)
  {
    // Create a new representation
    vtkSMProxy* newRepr = this->Internal->AddRepresentationClone(repr, iter->GetPointer());
    (void)newRepr;
    assert(newRepr != NULL);
  }

  if (this->OverlayAllComparisons)
  {
    size_t numReprs = this->Dimensions[0] * this->Dimensions[1];
    for (size_t cc = 1; cc < numReprs; cc++)
    {
      // Create a new representation
      vtkSMProxy* newRepr = this->Internal->AddRepresentationClone(repr, rootView);
      (void)newRepr;
      assert(newRepr);
    }
  }

  // Signal that representations have changed
  this->InvokeEvent(vtkCommand::UserEvent);

  // Override superclass' AddRepresentation since  repr has already been added
  // to the rootView.
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::RemoveRepresentation(vtkSMProxy* repr)
{
  ENSURE_INIT();

  vtkInternal::MapOfReprClones::iterator reprDataIter =
    this->Internal->RepresentationClones.find(repr);
  if (!repr || reprDataIter == this->Internal->RepresentationClones.end())
  {
    // Nothing to do.
    return;
  }

  this->MarkOutdated();

  vtkInternal::RepresentationData& data = reprDataIter->second;

  // Remove all clones of this representation.
  vtkInternal::RepresentationData::VectorOfClones::iterator cloneIter;
  for (cloneIter = data.Clones.begin(); cloneIter != data.Clones.end(); ++cloneIter)
  {
    vtkSMViewProxy* view = cloneIter->ViewProxy;
    vtkSMProxy* clone = cloneIter->CloneRepresentation;
    if (view && clone)
    {
      vtkRemoveRepresentation(view, clone);
      // No need to clean the clone from the proxy link since the link object
      // will be destroyed anyways.
    }
  }

  // This will destroy the repr proxy link as well.
  this->Internal->RepresentationClones.erase(reprDataIter);

  // Remove repr from RootView.
  vtkSMViewProxy* rootView = this->GetRootView();
  vtkRemoveRepresentation(rootView, repr);

  // Signal that representations have changed
  this->InvokeEvent(vtkCommand::UserEvent);

  // Override superclass' RemoveRepresentation since  repr was not added to this
  // view at all, we added it to (and removed from) the root view.
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::RemoveAllRepresentations()
{
  ENSURE_INIT();

  vtkInternal::MapOfReprClones::iterator iter = this->Internal->RepresentationClones.begin();
  while (iter != this->Internal->RepresentationClones.end())
  {
    vtkSMProxy* repr = iter->first;
    this->RemoveRepresentation(repr);
    iter = this->Internal->RepresentationClones.begin();
  }

  this->MarkOutdated();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::InteractiveRender()
{
  ENSURE_INIT();
  this->GetRootView()->InteractiveRender();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::StillRender()
{
  ENSURE_INIT();
  this->GetRootView()->StillRender();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::Update()
{
  if (!this->Outdated)
  {
    // cout << "Not Outdated" << endl;
    return;
  }

  vtkSMComparativeAnimationCueProxy* timeCue = NULL;
  // locate time cue.
  for (vtkInternal::VectorOfCues::iterator iter = this->Internal->Cues.begin();
       iter != this->Internal->Cues.end(); ++iter)
  {
    // for now, we are saying that the first cue that has no animatable  proxy
    // is for animating time.
    if (vtkSMPropertyHelper(iter->GetPointer(), "AnimatedProxy").GetAsProxy() == NULL)
    {
      timeCue = iter->GetPointer();
      break;
    }
  }

  int index = 0;
  for (int y = 0; y < this->Dimensions[1]; y++)
  {
    for (int x = 0; x < this->Dimensions[0]; x++)
    {
      int view_index = this->OverlayAllComparisons ? 0 : index;
      vtkSMViewProxy* view = this->Internal->Views[view_index];

      if (timeCue)
      {
        double value = timeCue->GetValue(x, y, this->Dimensions[0], this->Dimensions[1]);
        vtkSMPropertyHelper(view, "ViewTime").Set(value);
      }
      else
      {
        vtkSMPropertyHelper(view, "ViewTime").Set(this->ViewTime);
      }
      view->UpdateVTKObjects();

      for (vtkInternal::VectorOfCues::iterator iter = this->Internal->Cues.begin();
           iter != this->Internal->Cues.end(); ++iter)
      {
        if (iter->GetPointer() == timeCue)
        {
          continue;
        }
        iter->GetPointer()->UpdateAnimatedValue(x, y, this->Dimensions[0], this->Dimensions[1]);
      }

      // Make the view cache the current setup.
      this->UpdateAllRepresentations(x, y);
      index++;
    }
  }

  this->Outdated = false;
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::UpdateAllRepresentations(int x, int y)
{
  // Ensure cache is cleared for the representations corresponding to the given
  // position.
  this->ClearDataCaches(x, y);

  vtkSMViewProxy* view = this->OverlayAllComparisons
    ? this->Internal->Views[0]
    : this->Internal->Views[y * this->Dimensions[0] + x];
  // Simply update the corresponding view. That'll update the representations in
  // that view.
  view->Update();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::ClearDataCaches(int x, int y)
{
  // Mark all representations modified. This clears their caches. It's essential
  // that SetUseCache(false) is called before we do this, otherwise the caches
  // are not cleared.

  vtkInternal::MapOfReprClones::iterator repcloneiter;
  for (repcloneiter = this->Internal->RepresentationClones.begin();
       repcloneiter != this->Internal->RepresentationClones.end(); ++repcloneiter)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(repcloneiter->first);
    vtkSMRepresentationProxy* reprToClear = NULL;
    if (x == 0 && y == 0)
    {
      reprToClear = repr;
    }
    else
    {
      assert(
        static_cast<size_t>(x + this->Dimensions[0] * y - 1) < repcloneiter->second.Clones.size());
      const vtkInternal::RepresentationCloneItem& item =
        repcloneiter->second.Clones[x + this->Dimensions[0] * y - 1];
      reprToClear = vtkSMRepresentationProxy::SafeDownCast(item.CloneRepresentation.GetPointer());
    }

    if (reprToClear)
    {
      vtkSMPropertyHelper helper(reprToClear, "ForceUseCache", true);
      helper.Set(0);
      reprToClear->UpdateProperty("ForceUseCache");
      reprToClear->ClearMarkedModified(); // HACK.
      reprToClear->MarkDirty(NULL);
      helper.Set(1);
      reprToClear->UpdateProperty("ForceUseCache");
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::GetViews(vtkCollection* collection)
{
  if (!collection)
  {
    return;
  }

  vtkInternal::VectorOfViews::iterator iter;
  for (iter = this->Internal->Views.begin(); iter != this->Internal->Views.end(); ++iter)
  {
    collection->AddItem(iter->GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::GetRepresentationsForView(
  vtkSMViewProxy* view, vtkCollection* collection)
{
  if (!collection)
  {
    return;
  }

  // Find all representations in this view.

  // For each representation
  vtkInternal::MapOfReprClones::iterator reprIter;
  for (reprIter = this->Internal->RepresentationClones.begin();
       reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
  {

    if (view == this->GetRootView())
    {
      // If the view is the root view, then its representations
      // are the keys of map we are iterating over
      collection->AddItem(reprIter->first);
      continue;
    }

    // The view is not the root view, so it could be one of the cloned views.
    // Search the RepresentationData struct for a representation
    // belonging to the cloned view.
    vtkInternal::RepresentationData& data = reprIter->second;
    vtkInternal::RepresentationData::VectorOfClones::iterator cloneIter =
      data.FindRepresentationClone(view);
    if (cloneIter != data.Clones.end())
    {
      // A representation was found, so add it to the collection.
      vtkSMProxy* repr = cloneIter->CloneRepresentation;
      collection->AddItem(repr);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::GetRepresentations(int x, int y, vtkCollection* collection)
{
  if (!collection)
  {
    return;
  }

  vtkSMViewProxy* view = this->OverlayAllComparisons
    ? this->Internal->Views[0]
    : this->Internal->Views[y * this->Dimensions[0] + x];

  int index = y * this->Dimensions[0] + x;

  if (!this->OverlayAllComparisons)
  {
    this->GetRepresentationsForView(view, collection);
    return;
  }

  vtkInternal::MapOfReprClones::iterator reprIter;
  for (reprIter = this->Internal->RepresentationClones.begin();
       reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
  {
    vtkInternal::RepresentationData& data = reprIter->second;
    if (index == 0)
    {
      collection->AddItem(reprIter->first);
    }
    else
    {
      collection->AddItem(data.Clones[index - 1].CloneRepresentation);
    }
  }
}

//----------------------------------------------------------------------------
vtkImageData* vtkPVComparativeView::CaptureWindow(int magnification)
{
  std::vector<vtkSmartPointer<vtkImageData> > images;
  for (vtkInternal::VectorOfViews::const_iterator iter = this->Internal->Views.begin();
       iter != this->Internal->Views.end(); ++iter)
  {
    vtkImageData* image = iter->GetPointer()->CaptureWindow(magnification);
    if (image)
    {
      images.push_back(image);
      image->FastDelete();
    }
    if (this->OverlayAllComparisons)
    {
      break;
    }
  }

  if (images.size() == 0)
  {
    return NULL;
  }

  unsigned char color[3];
  vtkSMViewLayoutProxy::GetMultiViewImageBorderColor(color);
  vtkSmartPointer<vtkImageData> img = vtkSMUtilities::MergeImages(images, 0, color);
  if (img)
  {
    img->Register(this);
  }
  return img.GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensions: " << this->Dimensions[0] << ", " << this->Dimensions[1] << endl;
  os << indent << "Spacing: " << this->Spacing[0] << ", " << this->Spacing[1] << endl;
}
