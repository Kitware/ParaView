/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThreeSliceView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVThreeSliceView.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSmartPointer.h"

#include <vector>
#include <map>
#include <set>
#include <string>
#include <vtksys/ios/sstream>

#include <assert.h>

#define ENSURE_INIT() if (this->GetBottomRightView() == NULL) { return; }

//----------------------------------------------------------------------------
namespace
{
  static void vtkCopyClone(vtkSMProxy* source, vtkSMProxy* clone,
    std::set<std::string> *exceptions=0)
    {
    vtkSmartPointer<vtkSMPropertyIterator> iterSource;
    vtkSmartPointer<vtkSMPropertyIterator> iterDest;

    iterSource.TakeReference(source->NewPropertyIterator());
    iterDest.TakeReference(clone->NewPropertyIterator());

    // Since source/clone are exact copies, the following is safe.
    for (; !iterSource->IsAtEnd() && !iterDest->IsAtEnd();
      iterSource->Next(), iterDest->Next())
      {
      // Skip the property if it is in the exceptions list.
      if (exceptions &&
        exceptions->find(iterSource->GetKey()) != exceptions->end())
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
class vtkPVThreeSliceView::vtkInternal
{
public:
  struct RepresentationCloneItem
    {
    // The clone representation proxy.
    vtkSmartPointer<vtkSMProxy> CloneRepresentation;

    // The sub-view in which this clone exists.
    vtkSmartPointer<vtkSMRenderViewProxy> ViewProxy;

    RepresentationCloneItem() {}
    RepresentationCloneItem(
      vtkSMRenderViewProxy* view, vtkSMProxy* repr)
      : CloneRepresentation(repr), ViewProxy(view) {}
    };

  struct RepresentationData
    {
    typedef std::vector<RepresentationCloneItem> VectorOfClones;
    VectorOfClones Clones;
    vtkSmartPointer<vtkSMProxyLink> Link;

    void MarkRepresentationsModified()
      {
      VectorOfClones::iterator iter;
      for (iter = this->Clones.begin(); iter != this->Clones.end(); ++iter)
        {
        vtkSMRepresentationProxy * repr =
          vtkSMRepresentationProxy::SafeDownCast(iter->CloneRepresentation);
        if (repr)
          {
          vtkSMPropertyHelper helper(repr, "ForceUseCache", true);
          helper.Set(0);
          repr->UpdateProperty("ForceUseCache");
          repr->MarkDirty(NULL);
          helper.Set(1);
          repr->UpdateProperty("ForceUseCache");
          }
        }
      }

    // Returns the representation clone in the given view.
    VectorOfClones::iterator FindRepresentationClone(vtkSMRenderViewProxy* view)
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

  typedef std::vector<vtkSmartPointer<vtkSMRenderViewProxy> > VectorOfViews;
  VectorOfViews Views;

  typedef std::map<vtkSMProxy*, RepresentationData> MapOfReprClones;
  MapOfReprClones RepresentationClones;

  vtkSmartPointer<vtkSMProxyLink> ViewLink;

  vtkInternal()
    {
    this->ViewLink = vtkSmartPointer<vtkSMProxyLink>::New();
    }

  // Creates a new representation clone and adds it in the given view.
  // Arguments:
  // @repr -- representation to clone
  // @view -- the view to which the new clone should be added.
  vtkSMProxy* AddRepresentationClone(vtkSMProxy* repr, vtkSMRenderViewProxy* view)
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
    cout << " -> Create Rep: " << repr->GetXMLGroup() << " " << repr->GetXMLName() << endl;
    vtkSMProxy* newRepr =
      pxm->NewProxy(repr->GetXMLGroup(), repr->GetXMLName());

    // Made the new representation a clone
    vtkCopyClone(repr, newRepr);
    newRepr->UpdateVTKObjects();
    data.Link->AddLinkedProxy(newRepr, vtkSMLink::OUTPUT);

    // Add the cloned representation to the view
    vtkAddRepresentation(view, newRepr);

    // Add the cloned representation to the RepresentationData struct
    // The clone is added to a map where its view is the key.
    data.Clones.push_back(
      vtkInternal::RepresentationCloneItem(view, newRepr));

    // Clean up this reference
    newRepr->Delete();
    return newRepr;
    }

  std::string SuggestedViewType;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVThreeSliceView);
vtkCxxSetObjectMacro(vtkPVThreeSliceView, TopLeftView, vtkSMRenderViewProxy);
vtkCxxSetObjectMacro(vtkPVThreeSliceView, TopRightView, vtkSMRenderViewProxy);
vtkCxxSetObjectMacro(vtkPVThreeSliceView, BottomLeftView, vtkSMRenderViewProxy);
vtkCxxSetObjectMacro(vtkPVThreeSliceView, BottomRightView, vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkPVThreeSliceView::vtkPVThreeSliceView()
{
  this->Internal = new vtkInternal();
  this->TopLeftView = this->TopRightView
      = this->BottomLeftView = this->BottomRightView = NULL;
  this->ViewTime = 0.0;
  this->Outdated = true;

  vtkMemberFunctionCommand<vtkPVThreeSliceView>* fsO =
    vtkMemberFunctionCommand<vtkPVThreeSliceView>::New();
  fsO->SetCallback(*this, &vtkPVThreeSliceView::MarkOutdated);
  this->MarkOutdatedObserver = fsO;
}

//----------------------------------------------------------------------------
vtkPVThreeSliceView::~vtkPVThreeSliceView()
{
  this->SetTopLeftView(NULL);
  this->SetTopRightView(NULL);
  this->SetBottomLeftView(NULL);
  this->SetBottomRightView(NULL);

  delete this->Internal;
  this->MarkOutdatedObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::Initialize(
    vtkSMRenderViewProxy *topLeft, vtkSMRenderViewProxy *topRight,
    vtkSMRenderViewProxy *bottomLeft, vtkSMRenderViewProxy *bottomRight)
{
  if (this->TopLeftView == topLeft || !topLeft)
    {
    return;
    }

  if (this->TopLeftView)
    {
    vtkErrorMacro(
      "vtkPVThreeSliceView::Initialize() can only be called once.");
    return;
    }

  this->SetTopLeftView(topLeft);
  this->SetTopRightView(topRight);
  this->SetBottomLeftView(bottomLeft);
  this->SetBottomRightView(bottomRight);

  ENSURE_INIT();

  // Root view is the first view in the views list.
  this->Internal->Views.push_back(bottomRight);
  this->Internal->ViewLink->AddLinkedProxy(bottomRight, vtkSMLink::INPUT);

  // Every view keeps their own representations.
  this->Internal->ViewLink->AddException("Representations");

  // This view computes view size/view position for each view based on the
  // layout.
  //this->Internal->ViewLink->AddException("ViewSize");
  //this->Internal->ViewLink->AddException("ViewTime");
  this->Internal->ViewLink->AddException("CacheKey");
  this->Internal->ViewLink->AddException("UseCache");
  //this->Internal->ViewLink->AddException("ViewPosition");

  this->Internal->ViewLink->AddException("CameraPositionInfo");
  this->Internal->ViewLink->AddException("CameraPosition");
  this->Internal->ViewLink->AddException("CameraFocalPointInfo");
  this->Internal->ViewLink->AddException("CameraFocalPoint");
  this->Internal->ViewLink->AddException("CameraViewUpInfo");
  this->Internal->ViewLink->AddException("CameraViewUp");
  this->Internal->ViewLink->AddException("CameraClippingRangeInfo");
  this->Internal->ViewLink->AddException("CameraClippingRange");
  this->Internal->ViewLink->AddException("CameraViewAngleInfo");
  this->Internal->ViewLink->AddException("CameraViewAngle");


  // --- Register slice views
  std::set<std::string> exceptions;
  exceptions.insert("Representations");
  //exceptions.insert("ViewSize");
  exceptions.insert("UseCache");
  exceptions.insert("CacheKey");
  //exceptions.insert("ViewPosition");

  // TopLeft Slice
  vtkCopyClone(bottomRight, topLeft, &exceptions);
  this->Internal->Views.push_back(topLeft);
  this->Internal->ViewLink->AddLinkedProxy(topLeft, vtkSMLink::OUTPUT);

  // TopRight Slice
  vtkCopyClone(bottomRight, topRight, &exceptions);
  this->Internal->Views.push_back(topRight);
  this->Internal->ViewLink->AddLinkedProxy(topRight, vtkSMLink::OUTPUT);

  // BottomLeft Slice
  vtkCopyClone(bottomRight, bottomLeft, &exceptions);
  this->Internal->Views.push_back(bottomLeft);
  this->Internal->ViewLink->AddLinkedProxy(bottomLeft, vtkSMLink::OUTPUT);
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::AddRepresentation(vtkSMProxy* repr)
{
  cout << "AddRepresentation " << repr << endl;
  ENSURE_INIT();
  cout << "- ok" << endl;

  if (!repr)
    {
    return;
    }

  this->MarkOutdated();

  // Add representation to the root view
  vtkSMRenderViewProxy* rootView = this->GetBottomRightView();
  vtkAddRepresentation(rootView, repr);

  // Create clones of representation and add them to all other views.
  // We will save information about the clones we create
  // so that we can clean them up later.
  vtkInternal::RepresentationData data;

  // We'll link the clones' properties to the original
  // representation using a proxy link.  The "UpdateTime"
  // property will not be linked however.
  vtkNew<vtkSMProxyLink> reprLink;
  data.Link = reprLink.GetPointer();
  reprLink->AddLinkedProxy(repr, vtkSMLink::INPUT);
  reprLink->AddException("ForceUseCache");
  reprLink->AddException("ForcedCacheKey");
  reprLink->AddException("OutputType");

  // Add the RepresentationData struct to a map
  // with the original representation as the key
  this->Internal->RepresentationClones[repr] = data;

  // Now, for all existing sub-views, create representation clones.
  vtkInternal::VectorOfViews::iterator iter = this->Internal->Views.begin();
  iter++; // skip root view.
  int portToUse = 1;
  for (; iter != this->Internal->Views.end(); ++iter)
    {
    // Create a new representation
    vtkSMProxy* newRepr =
        this->Internal->AddRepresentationClone(repr, iter->GetPointer());
    vtkSMPropertyHelper(newRepr, "OutputType").Set(portToUse++);
    newRepr->UpdateVTKObjects();
    (void)newRepr;
    assert(newRepr != NULL);
    }

  // Signal that representations have changed
  this->InvokeEvent(vtkCommand::UserEvent);

  // Override superclass' AddRepresentation since  repr has already been added
  // to the rootView.
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::RemoveRepresentation(vtkSMProxy* repr)
{
  cout << "RemoveRepresentation " << repr << endl;
  ENSURE_INIT();
  cout << "- ok" << endl;

  vtkInternal::MapOfReprClones::iterator reprDataIter
    = this->Internal->RepresentationClones.find(repr);
  if (!repr || reprDataIter == this->Internal->RepresentationClones.end())
    {
    // Nothing to do.
    cout << "Nothing found..." << endl;
    return;
    }

  this->MarkOutdated();

  vtkInternal::RepresentationData& data = reprDataIter->second;

  // Remove all clones of this representation.
  vtkInternal::RepresentationData::VectorOfClones::iterator cloneIter;
  for (cloneIter = data.Clones.begin(); cloneIter != data.Clones.end(); ++cloneIter)
    {
    vtkSMRenderViewProxy* view = cloneIter->ViewProxy;
    vtkSMProxy* clone = cloneIter->CloneRepresentation;
    if (view && clone)
      {
      vtkRemoveRepresentation(view, clone);
      // No need to clean the clone from the proxy link since the link object
      // will be destroyed anyways.
      cout << "Remove a clone rep" << endl;
      }
    }

  // This will destroy the repr proxy link as well.
  this->Internal->RepresentationClones.erase(reprDataIter);

  // Remove repr from RootView.
  vtkSMRenderViewProxy* rootView = this->GetBottomRightView();
  vtkRemoveRepresentation(rootView, repr);

  // Signal that representations have changed
  this->InvokeEvent(vtkCommand::UserEvent);

  // Override superclass' RemoveRepresentation since  repr was not added to this
  // view at all, we added it to (and removed from) the root view.
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::RemoveAllRepresentations()
{
  cout << "RemoveAllRepresentations " << endl;
  ENSURE_INIT();
  cout << "- ok" << endl;

  vtkInternal::MapOfReprClones::iterator iter =
    this->Internal->RepresentationClones.begin();
  while (iter != this->Internal->RepresentationClones.end())
    {
    vtkSMProxy* repr = iter->first;
    this->RemoveRepresentation(repr);
    iter = this->Internal->RepresentationClones.begin();
    }

  this->MarkOutdated();
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::InteractiveRender()
{
  ENSURE_INIT();
  this->GetBottomRightView()->InteractiveRender();
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::StillRender()
{
  ENSURE_INIT();
  this->GetBottomRightView()->StillRender();
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::Update()
{
//  if (!this->Outdated)
//    {
//    cout << "Not Outdated" << endl;
//    return;
//    }

  this->UpdateAllRepresentations();
  this->ClearDataCaches();

  this->Outdated = false;
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::UpdateAllRepresentations()
{
  cout << "UpdateAllRepresentations" << endl;
  for(int i = 0; i < 4; ++i)
    {
    vtkSMRenderViewProxy* view = this->Internal->Views[i];

    vtkCollection* collection = vtkCollection::New();
    this->GetRepresentationsForView(view, collection);
    collection->InitTraversal();
    while (vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(
        collection->GetNextItemAsObject()))
      {
      if (vtkSMPropertyHelper(repr, "Visibility", true).GetAsInt() == 1)
        {
        cout << "UpdatePipeline of " << repr->GetXMLName() << endl;
       std::vector<double> xSlices = vtkSMPropertyHelper(repr, "XSlicesValues", true).GetDoubleArray();
       cout << "Number of slice in X " << xSlices.size() << endl;

        repr->UpdatePipeline(this->ViewTime);
        repr->MarkModified(NULL);
        repr->MarkDirty(NULL);
        repr->UpdateVTKObjects();
        }
      }
    view->Update();

    }

}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::ClearDataCaches()
{
  // Mark all representations modified. This clears their caches. It's essential
  // that SetUseCache(false) is called before we do this, otherwise the caches
  // are not cleared.

  vtkInternal::MapOfReprClones::iterator repcloneiter;
  for (repcloneiter = this->Internal->RepresentationClones.begin();
    repcloneiter != this->Internal->RepresentationClones.end();
    ++repcloneiter)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      repcloneiter->first);
    if (repr)
      {
      vtkSMPropertyHelper helper(repr, "ForceUseCache", true);
      helper.Set(0);
      repr->UpdateProperty("ForceUseCache");
      // HACK.
      // FIXME seb repr->ClearMarkedModified();
      repr->MarkDirty(NULL);
      repcloneiter->second.MarkRepresentationsModified();
      helper.Set(1);
      repr->UpdateProperty("ForceUseCache");
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::GetRepresentationsForView(vtkSMRenderViewProxy* view,
      vtkCollection *collection)
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

    if (view == this->GetBottomRightView())
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
    vtkInternal::RepresentationData::VectorOfClones::iterator cloneIter
      = data.FindRepresentationClone(view);
    if (cloneIter != data.Clones.end())
      {
      // A representation was found, so add it to the collection.
      vtkSMProxy* repr = cloneIter->CloneRepresentation;
      collection->AddItem(repr);
      }
    }

}

//----------------------------------------------------------------------------
void vtkPVThreeSliceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
