
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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkProcessModule.h"
#include "vtkSMCameraLink.h"
#include "vtkSMComparativeAnimationCueProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

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
  assert(start == 0 && count >= 1 && pos >= 0 && space >= 0 && spacing >= 0);
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
namespace vtkPVComparativeViewNS
{
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

//----------------------------------------------------------------------------
/**
 * @class vtkCloningVector
 * @brief base class for a vector that resizes itself with clones of the "root".
 *
 * vtkCloningVector is allows to create and maintain clones for any proxy. The vector can be
 * resized to any size (> 1) and it will fill itself will clones of the root proxy (at index 0).
 * In keeps properties on the clones linked so that when the properties on the root change,
 * the clones' properties also change.
 *
 */
//----------------------------------------------------------------------------
class vtkCloningVector : public vtkObject
{
public:
  vtkTypeMacro(vtkCloningVector, vtkObject);

  /**
   * @returns item at a given index, if valid, else nullptr is returned.
   */
  vtkSMProxy* GetItem(size_t index) const
  {
    return (index < this->Items.size()) ? this->Items[index].GetPointer() : nullptr;
  }

  /**
   * @returns root proxy
   */
  vtkSMProxy* GetRoot() const { return this->GetItem(0); }

  /**
   * @returns last proxy or nullptr if the vector hasn't been initialized yet.
   */
  vtkSMProxy* GetBack() const
  {
    return (this->Items.size() > 0) ? this->Items.back().GetPointer() : nullptr;
  }

  /**
   * @returns number of items
   */
  size_t GetNumberOfItems() const { return this->Items.size(); }

  /**
   * Resize to the given size (>=1). Any extra proxies will be removed by iteratively calling
   * this->PopBack(). New proxies may be added by iteratively calling this->PushBack().
   *
   * @returns true if anything changed, otherwise false.
   */
  virtual bool Resize(size_t num_items)
  {
    assert(num_items >= 1);
    bool changed = false;

    // Remove extra items.
    for (size_t cc = this->Items.size() - 1; cc >= num_items; --cc)
    {
      this->PopBack();
      changed = true;
    }

    // Add new items if needed.
    for (size_t cc = this->Items.size(); cc < num_items; ++cc)
    {
      this->PushBack();
      changed = true;
    }

    assert(this->GetNumberOfItems() == num_items);
    return changed;
  }

  /**
   * Create a new clone of the root and append it to the vector.
   */
  virtual vtkSMProxy* PushBack()
  {
    vtkSMProxy* root = this->GetRoot();
    assert(root);

    vtkSMSessionProxyManager* pxm = root->GetSessionProxyManager();
    vtkSmartPointer<vtkSMProxy> clone =
      vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy(root->GetXMLGroup(), root->GetXMLName()));
    if (clone)
    {
      vtkNew<vtkSMParaViewPipelineController> controller;
      controller->PreInitializeProxy(clone);
      this->Copy(root, clone);
      controller->PostInitializeProxy(clone);
      clone->UpdateVTKObjects();

      this->Link->AddLinkedProxy(clone, vtkSMLink::OUTPUT);
      this->Items.push_back(clone);

      // Handle proxy-list domain proxies. Link proxies in the proxy list domains on the clone
      // with those in the corresponding domains on the root.
      for (auto iter = this->PLDLinks.begin(); iter != this->PLDLinks.end(); ++iter)
      {
        vtkSMProperty* prop = clone->GetProperty(iter->first.c_str());
        auto pld = prop ? prop->FindDomain<vtkSMProxyListDomain>() : nullptr;
        if (pld)
        {
          for (int cc = 0, max = pld->GetNumberOfProxies(); cc < max; ++cc)
          {
            if (cc < static_cast<int>(iter->second.size()))
            {
              this->Copy(iter->second[cc]->GetLinkedProxy(0), pld->GetProxy(cc));
              iter->second[cc]->AddLinkedProxy(pld->GetProxy(cc), vtkSMLink::OUTPUT);
            }
          }
        }
      }
    }

    return clone;
  }

  /**
   * Remove the last element in the vector.
   * This will never remove the root (or 0th element) on a initialized vector.
   */
  virtual void PopBack()
  {
    if (this->Items.size() <= 1)
    {
      // never pop the root.
      return;
    }

    vtkSmartPointer<vtkSMProxy> back = this->Items.back();
    assert(back);

    // Handle proxy-list domain. Remove proxies in proxy-list-domain on the proxy being removed
    // from the PLDLinks.
    for (auto iter = this->PLDLinks.begin(); iter != this->PLDLinks.end(); ++iter)
    {
      vtkSMProperty* prop = back->GetProperty(iter->first.c_str());
      auto pld = prop ? prop->FindDomain<vtkSMProxyListDomain>() : nullptr;
      if (pld)
      {
        for (int cc = 0, max = pld->GetNumberOfProxies(); cc < max; ++cc)
        {
          if (cc < static_cast<int>(iter->second.size()))
          {
            iter->second[cc]->RemoveLinkedProxy(pld->GetProxy(cc));
          }
        }
      }
    }

    this->Link->RemoveLinkedProxy(back);
    this->Items.pop_back();
  }

protected:
  vtkCloningVector() {}
  ~vtkCloningVector() override {}

  /**
   * This must be called to initialize the vector with the "root".
   * \c exceptions is used to indicate the properties that should be
   * linked together.
   */
  void Setup(vtkSMProxy* root, const std::set<std::string>& exceptions)
  {
    assert(root != nullptr);
    assert(this->Items.size() == 0);

    this->Items.resize(1);
    this->Items[0] = root;
    for (auto iter = exceptions.begin(); iter != exceptions.end(); ++iter)
    {
      this->Link->AddException(iter->c_str());
    }
    this->Link->AddLinkedProxy(root, vtkSMLink::INPUT);
    this->Exceptions = exceptions;

    // For all properties with proxy-list domains, we have to link the
    // proxies in those domains too.
    vtkSmartPointer<vtkSMPropertyIterator> iter =
      vtkSmartPointer<vtkSMPropertyIterator>::Take(root->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      vtkSMProperty* prop = iter->GetProperty();
      auto pld = prop ? prop->FindDomain<vtkSMProxyListDomain>() : nullptr;
      if (!pld)
      {
        continue;
      }

      std::vector<vtkSmartPointer<vtkSMProxyLink> >& links = this->PLDLinks[iter->GetKey()];
      links.resize(pld->GetNumberOfProxies());
      for (int cc = 0, max = pld->GetNumberOfProxies(); cc < max; ++cc)
      {
        vtkNew<vtkSMProxyLink> alink;
        alink->AddLinkedProxy(pld->GetProxy(cc), vtkSMLink::INPUT);
        links[cc] = alink.Get();
      }
    }
  }

private:
  vtkCloningVector(const vtkCloningVector&) = delete;
  void operator=(const vtkCloningVector&) = delete;

  /**
   * Copy all properties from source to clone, excluding the ones in
   * this->Exceptions.
   */
  void Copy(vtkSMProxy* source, vtkSMProxy* clone)
  {
    vtkSmartPointer<vtkSMPropertyIterator> iterSource;
    vtkSmartPointer<vtkSMPropertyIterator> iterDest;

    iterSource.TakeReference(source->NewPropertyIterator());
    iterDest.TakeReference(clone->NewPropertyIterator());

    // Since source/clone are exact copies, the following is safe.
    for (; !iterSource->IsAtEnd() && !iterDest->IsAtEnd(); iterSource->Next(), iterDest->Next())
    {
      // Skip the property if it is in the exceptions list.
      if (this->Exceptions.find(iterSource->GetKey()) != this->Exceptions.end())
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

      // For proxy properties with proxy-list domains, aka enumeration-properties
      // with proxies for values, we need to link the proxies in the domain too.
    }
  }

  std::vector<vtkSmartPointer<vtkSMProxy> > Items;
  vtkNew<vtkSMProxyLink> Link;
  std::set<std::string> Exceptions;
  // key: property name
  // value: ordered list of vtkSMProxyLinks for each of the proxies in the proxy list domain.
  std::map<std::string, std::vector<vtkSmartPointer<vtkSMProxyLink> > > PLDLinks;
};

//----------------------------------------------------------------------------
/**
 * @class vtkCloningVectorOfRepresentations
 * @brief vtkCloningVector specialization for representations.
 */
//----------------------------------------------------------------------------

class vtkCloningVectorOfRepresentations : public vtkCloningVector
{
public:
  vtkTypeMacro(vtkCloningVectorOfRepresentations, vtkCloningVector);
  static vtkCloningVectorOfRepresentations* New()
  {
    VTK_STANDARD_NEW_BODY(vtkCloningVectorOfRepresentations);
  }

  /**
   * Clear cached data for all representations.
   */
  void ClearDataCaches(size_t index)
  {
    // Mark all representations modified. This clears their caches. It's essential
    // that SetUseCache(false) is called before we do this, otherwise the caches
    // are not cleared.
    if (vtkSMRepresentationProxy* reprToClear =
          vtkSMRepresentationProxy::SafeDownCast(this->GetItem(index)))
    {
      vtkSMPropertyHelper helper(reprToClear, "ForceUseCache", true);
      helper.Set(0);
      reprToClear->UpdateProperty("ForceUseCache");
      reprToClear->ClearMarkedModified(); // HACK.
      reprToClear->MarkDirtyFromProducer(nullptr, nullptr, nullptr);
      helper.Set(1);
      reprToClear->UpdateProperty("ForceUseCache");
    }
  }

  /**
   * Initializes the instance with the given root representation.
   */
  void Initialize(vtkSMProxy* root)
  {
    std::set<std::string> exceptions = { "ForceUseCache", "ForceCacheKey" };
    this->Setup(root, exceptions);
  }

protected:
  vtkCloningVectorOfRepresentations() {}
  ~vtkCloningVectorOfRepresentations() override {}

private:
  vtkCloningVectorOfRepresentations(const vtkCloningVectorOfRepresentations&) = delete;
  void operator=(const vtkCloningVectorOfRepresentations&) = delete;
};

//----------------------------------------------------------------------------
/**
 * @class vtkCloningVectorOfViews
 * @brief vtkCloningVector specialization for views.
 *
 * vtkCloningVectorOfViews is a vtkCloningVector specialization for views. It also handles
 * cloning of representations for clones of the views.
 */
//----------------------------------------------------------------------------
class vtkCloningVectorOfViews : public vtkCloningVector
{
public:
  vtkTypeMacro(vtkCloningVectorOfViews, vtkCloningVector);
  static vtkCloningVectorOfViews* New() { VTK_STANDARD_NEW_BODY(vtkCloningVectorOfViews); }

  /*
   * Must be set to true if all comparisons are to be shown in the same view.
   */
  void SetOverlayViews(bool val) { this->OverlayViews = val; }

  /**
   * Type this->GetItem()
   */
  vtkSMViewProxy* GetView(size_t index) const
  {
    return vtkSMViewProxy::SafeDownCast(this->GetItem(index));
  }

  /**
   * Initialize instance with a root view
   */
  void Initialize(vtkSMViewProxy* root)
  {
    std::set<std::string> exceptions = { // Every view keeps their own representations.
      "Representations",

      // This view computes view size/view position for each view based on the
      // layout.
      "ViewSize", "ViewTime", "CacheKey", "UseCache", "ViewPosition",
      // Camera is linked via CameraLink.
      "CameraPositionInfo", "CameraPosition", "CameraFocalPointInfo", "CameraFocalPoint",
      "CameraViewUpInfo", "CameraViewUp", "CameraViewAngleInfo", "CameraViewAngle",
      "CameraFocalDiskInfo", "CameraFocalDisk", "CameraFocalDistanceInfo", "CameraFocalDistance"
    };

    this->Setup(root, exceptions);
    this->CameraLink->AddLinkedProxy(root, vtkSMLink::INPUT);
    this->CameraLink->AddLinkedProxy(root, vtkSMLink::OUTPUT);
    this->NumberOfComparisons = 1;
  }

  /**
   * Overridden to create/delete representations as views are added/removed.
   * Also handle this->OverlayViews == true.
   */
  bool Resize(size_t num_comparisons) override
  {
    this->NumberOfComparisons = num_comparisons;
    if (this->OverlayViews)
    {
      // We'll create 1 view, but multiple sets of representations for each comparison.
      vtkSMProxy* rootView = this->GetRoot();
      bool changed = this->Superclass::Resize(1);
      for (auto iter = this->Representations.begin(); iter != this->Representations.end(); ++iter)
      {
        size_t old_size = (*iter)->GetNumberOfItems();
        for (size_t cc = num_comparisons; cc < old_size; ++cc)
        {
          vtkRemoveRepresentation(rootView, (*iter)->GetItem(cc));
          changed = true;
        }
        changed = (*iter)->Resize(num_comparisons) || changed;
        for (size_t cc = old_size; cc < num_comparisons; ++cc)
        {
          vtkAddRepresentation(rootView, (*iter)->GetItem(cc));
          changed = true;
        }
      }
      return changed;
    }
    else
    {
      return this->Superclass::Resize(num_comparisons);
    }
  }

  /**
   * Add a representation to the root view. Create clones of the same and add them to
   * the comparison views (or same root view if this->OverlayViews == true
   */
  void AddRepresentation(vtkSMProxy* repr)
  {
    vtkSMViewProxy* rootView = this->GetView(0);

    vtkNew<vtkCloningVectorOfRepresentations> reprClones;
    reprClones->Initialize(repr);
    reprClones->Resize(this->NumberOfComparisons);
    for (size_t cc = 0; cc < this->NumberOfComparisons; ++cc)
    {
      vtkAddRepresentation(
        this->OverlayViews ? rootView : this->GetView(cc), reprClones->GetItem(cc));
    }
    this->Representations.push_back(reprClones.Get());
  }

  /**
   * Converse of AddRepresentation, to remove a representation and its clones.
   */
  void RemoveRepresentation(vtkSMProxy* repr)
  {
    vtkSMViewProxy* rootView = this->GetView(0);
    for (auto iter = this->Representations.begin(); iter != this->Representations.end(); ++iter)
    {
      if ((*iter)->GetRoot() == repr)
      {
        // found it! Let's remove it.
        for (size_t cc = 0; cc < this->NumberOfComparisons; ++cc)
        {
          vtkRemoveRepresentation(
            this->OverlayViews ? rootView : this->GetView(cc), (*iter)->GetItem(cc));
        }
        this->Representations.erase(iter);
        break;
      }
    }
  }

  void Update(size_t index)
  {
    // Ensure cache is cleared for the representations corresponding to the given
    // position.
    for (auto iter = this->Representations.begin(); iter != this->Representations.end(); ++iter)
    {
      (*iter)->ClearDataCaches(index);
    }

    vtkSMViewProxy* view = this->GetView(this->OverlayViews ? 0 : index);
    // Simply update the corresponding view. That'll update the representations in
    // that view.
    view->Update();
  }

protected:
  vtkCloningVectorOfViews()
    : OverlayViews(false)
    , NumberOfComparisons(0)
  {
    this->CameraLink->SynchronizeInteractiveRendersOff();
  }

  ~vtkCloningVectorOfViews() override {}

  /**
   * A new view is being created, we need to create clones of representations too
   * and add them to the new view.
   */
  vtkSMProxy* PushBack() override
  {
    if (vtkSMProxy* clone = this->Superclass::PushBack())
    {
      this->CameraLink->AddLinkedProxy(clone, vtkSMLink::INPUT);
      this->CameraLink->AddLinkedProxy(clone, vtkSMLink::OUTPUT);

      // todo: do I need to sync camera explicitly? -- YES!

      // clone representations for this view.
      for (auto iter = this->Representations.begin(); iter != this->Representations.end(); ++iter)
      {
        vtkSMProxy* reprClone = (*iter)->PushBack();
        vtkAddRepresentation(clone, reprClone);
      }
      return clone;
    }
    else
    {
      vtkGenericWarningMacro("Failed to create internal view proxy. Comparative visualization "
                             "view cannot work.");
      return nullptr;
    }
  }

  /**
   * A view is being removed; we need to remove clones of representations for that view too.
   */
  void PopBack() override
  {
    vtkSMProxy* back = this->GetBack();
    if (back == this->GetRoot())
    {
      vtkGenericWarningMacro("Cannot remove the root view!");
      return;
    }

    // remove repr clones.
    for (auto iter = this->Representations.begin(); iter != this->Representations.end(); ++iter)
    {
      vtkSMProxy* reprBack = (*iter)->GetBack();
      vtkRemoveRepresentation(back, reprBack);
      (*iter)->PopBack();
    }
    this->CameraLink->RemoveLinkedProxy(back);
    this->Superclass::PopBack();
  }

private:
  vtkCloningVectorOfViews(const vtkCloningVectorOfViews&) = delete;
  void operator=(const vtkCloningVectorOfViews&) = delete;

  vtkNew<vtkSMCameraLink> CameraLink;
  bool OverlayViews;

  size_t NumberOfComparisons;
  std::vector<vtkSmartPointer<vtkCloningVectorOfRepresentations> > Representations;
};
}

//----------------------------------------------------------------------------
class vtkPVComparativeView::vtkInternal
{
public:
  typedef std::vector<vtkSmartPointer<vtkSMComparativeAnimationCueProxy> > VectorOfCues;
  VectorOfCues Cues;

  vtkNew<vtkPVComparativeViewNS::vtkCloningVectorOfViews> Views;
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
  this->Internal->Views->Initialize(rootView);

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

  this->Internal->Views->SetOverlayViews(this->OverlayAllComparisons);
  size_t numComparisons = dx * dy;
  assert(numComparisons >= 1);

  if (this->Internal->Views->Resize(numComparisons))
  {
    this->MarkOutdated();
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
      vtkSMViewProxy* view = this->Internal->Views->GetView(view_index);

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
void vtkPVComparativeView::AddRepresentation(vtkSMProxy* repr)
{
  ENSURE_INIT();

  if (!repr)
  {
    return;
  }

  this->MarkOutdated();

  this->Internal->Views->AddRepresentation(repr);

  // Signal that representations have changed
  this->InvokeEvent(vtkCommand::UserEvent);
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::RemoveRepresentation(vtkSMProxy* repr)
{
  ENSURE_INIT();

  this->MarkOutdated();

  this->Internal->Views->RemoveRepresentation(repr);

  // Signal that representations have changed
  this->InvokeEvent(vtkCommand::UserEvent);
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
      vtkSMViewProxy* view = this->Internal->Views->GetView(view_index);
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
      this->Internal->Views->Update(index);
      index++;
    }
  }

  this->Outdated = false;
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::GetViews(vtkCollection* collection)
{
  if (!collection)
  {
    return;
  }

  for (size_t cc = 0, max = this->Internal->Views->GetNumberOfItems(); cc < max; ++cc)
  {
    collection->AddItem(this->Internal->Views->GetItem(cc));
  }
}

//----------------------------------------------------------------------------
vtkImageData* vtkPVComparativeView::CaptureWindow(int magX, int magY)
{
  std::vector<vtkSmartPointer<vtkImageData> > images;

  vtkPVComparativeViewNS::vtkCloningVectorOfViews* views = this->Internal->Views.Get();
  for (size_t cc = 0, max = views->GetNumberOfItems(); cc < max; ++cc)
  {
    vtkImageData* image = views->GetView(cc)->CaptureWindow(magX, magY);
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

  const unsigned char color[3] = { 0, 0, 0 };
  vtkSmartPointer<vtkImageData> img = vtkSMUtilities::MergeImages(images, /*borderWidth=*/0, color);
  if (img)
  {
    img->Register(this);
  }
  return img.GetPointer();
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::SetTileScale(int, int)
{
  // we don't do anything here since `CaptureWindow` will handle scaling of each
  // view separately.
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::SetTileViewport(double, double, double, double)
{
  // we don't do anything here since `CaptureWindow` will handle scaling of each
  // view separately.
}

//----------------------------------------------------------------------------
void vtkPVComparativeView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensions: " << this->Dimensions[0] << ", " << this->Dimensions[1] << endl;
  os << indent << "Spacing: " << this->Spacing[0] << ", " << this->Spacing[1] << endl;
}
