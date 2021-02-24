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
#include "vtkSMViewLayoutProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkRect.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkViewLayout.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <queue>

class vtkSMViewLayoutProxy::vtkInternals
{
public:
  struct Cell
  {
    vtkSMViewLayoutProxy::Direction Direction;
    double SplitFraction;
    vtkVector4d Viewport;
    vtkWeakPointer<vtkSMViewProxy> ViewProxy;

    Cell()
      : Direction(vtkSMViewLayoutProxy::NONE)
      , SplitFraction(0.5)
      , Viewport(0, 0, 0, 0)
    {
    }
  };

  bool IsCellValid(int location)
  {
    if (location < 0 || location >= static_cast<int>(this->KDTree.size()))
    {
      return false;
    }

    if (location == 0)
    {
      return true;
    }

    // now verify that every parent node for location is a split cell.
    int parent = (static_cast<int>(location) - 1) / 2;
    while (this->KDTree[parent].Direction != vtkSMViewLayoutProxy::NONE)
    {
      if (parent == 0)
      {
        return true;
      }

      parent = (static_cast<int>(parent) - 1) / 2;
    }

    return false;
  }

  void MoveSubtree(int destination, int source)
  {
    assert(destination >= 0 && source >= 0);

    // we only support moving a subtree "up".
    assert(destination < source);

    if (source >= static_cast<int>(this->KDTree.size()) ||
      destination >= static_cast<int>(this->KDTree.size()))
    {
      return;
    }

    // Do a breadth-first traversal of source and destination. This is to ensure that
    // we swap elements in the KDTree in the proper order (moving from lower elements
    // in the tree up) when moving the subtree up the tree.
    std::queue<int> sourceQueue;
    std::queue<int> destQueue;

    sourceQueue.push(source);
    destQueue.push(destination);

    int kdtreeSize = static_cast<int>(this->KDTree.size());

    while (!sourceQueue.empty())
    {
      int currentSource = sourceQueue.front();
      sourceQueue.pop();
      int currentDest = destQueue.front();
      destQueue.pop();

      // Copy source to destination.
      if (currentSource < kdtreeSize && currentDest < kdtreeSize)
      {
        this->KDTree[currentDest] = this->KDTree[currentSource];
        this->KDTree[currentSource] = Cell();

        // Push children onto queue
        sourceQueue.push(GetFirstChild(currentSource));
        sourceQueue.push(GetSecondChild(currentSource));
        destQueue.push(GetFirstChild(currentDest));
        destQueue.push(GetSecondChild(currentDest));
      }
    }
  }

  void Shrink()
  {
    size_t max_index = this->GetMaxChildIndex(0);
    assert(max_index < this->KDTree.size());
    this->KDTree.resize(max_index + 1);
  }

  void UpdateViewports(int index = 0, const vtkVector4d& parentViewport = vtkVector4d(0, 0, 1, 1))
  {
    Cell& cell = this->KDTree[index];
    cell.Viewport = parentViewport;
    if (cell.Direction != vtkSMViewLayoutProxy::NONE)
    {
      cell.Viewport = parentViewport;

      const int dim = cell.Direction == vtkSMViewLayoutProxy::HORIZONTAL ? 0 : 1;
      const double delta1 = (parentViewport[2 + dim] - parentViewport[dim]) * cell.SplitFraction;
      const double delta2 = (parentViewport[2 + dim] - parentViewport[dim]) - delta1;

      vtkVector4d vp1(parentViewport), vp2(parentViewport);
      vp1[2 + dim] = vp1[dim] + delta1;
      vp2[dim] = vp1[2 + dim];
      vp2[2 + dim] = vp2[dim] + delta2;
      this->UpdateViewports(vtkSMViewLayoutProxy::GetFirstChild(index), vp1);
      this->UpdateViewports(vtkSMViewLayoutProxy::GetSecondChild(index), vp2);
    }
  }

  void UpdateViewPositions(int spacing, int root = 0, int posx = 0, int posy = 0)
  {
    if (root == 0)
    {
      this->Sizes.resize(this->KDTree.size() * 2);
      this->ComputeSizes(root, spacing);
    }

    const Cell& cell = this->KDTree[root];
    if (cell.Direction == vtkSMViewLayoutProxy::NONE)
    {
      if (cell.ViewProxy)
      {
        int pos[2] = { posx, posy };
        vtkSMPropertyHelper(cell.ViewProxy, "ViewPosition").Set(pos, 2);
        cell.ViewProxy->UpdateProperty("ViewPosition");
      }
      // cout << "View Position: " << cell.ViewProxy  << " = "
      //  << posx << "," << posy << endl;
    }
    else
    {
      // root is a split-cell. Determine sizes for the two children.
      const int* size = &this->Sizes[2 * (2 * root + 1)];

      if (cell.Direction == vtkSMViewLayoutProxy::HORIZONTAL)
      {
        this->UpdateViewPositions(spacing, 2 * root + 1, posx, posy);
        this->UpdateViewPositions(spacing, 2 * root + 2, posx + size[0] + spacing, posy);
      }
      else // cell.Direction == VERTICAL
      {
        this->UpdateViewPositions(spacing, 2 * root + 1, posx, posy);
        this->UpdateViewPositions(spacing, 2 * root + 2, posx, posy + size[1] + spacing);
      }
    }
  }

  void ResizeCell(int spacing, int root, const vtkVector2i& new_size)
  {
    if (root >= static_cast<int>(this->KDTree.size()))
    {
      return;
    }

    Cell& cell = this->KDTree[root];
    if (cell.Direction == vtkSMViewLayoutProxy::NONE)
    {
      if (cell.ViewProxy)
      {
        vtkSMPropertyHelper(cell.ViewProxy, "ViewSize").Set(new_size.GetData(), 2);
        cell.ViewProxy->UpdateProperty("ViewSize");
      }
    }
    else
    {
      // distribute size by fraction.
      const int lspacing = spacing / 2;
      const int rspacing = spacing - lspacing;

      vtkVector2i size_1(new_size), size_2(new_size);
      const int index = cell.Direction == vtkSMViewLayoutProxy::HORIZONTAL ? 0 : 1;
      const auto delta0 = std::ceil(new_size[index] * cell.SplitFraction);
      const auto delta1 = new_size[index] - delta0;
      size_1[index] = delta0 - lspacing;
      size_2[index] = delta1 - rspacing;
      this->ResizeCell(spacing, vtkSMViewLayoutProxy::GetFirstChild(root), size_1);
      this->ResizeCell(spacing, vtkSMViewLayoutProxy::GetSecondChild(root), size_2);
    }
  }

  typedef std::vector<Cell> KDTreeType;
  KDTreeType KDTree;
  vtkCommand* Observer;

private:
  size_t GetMaxChildIndex(size_t parent)
  {
    if (this->KDTree[parent].Direction == vtkSMViewLayoutProxy::NONE)
    {
      return parent;
    }

    size_t child1 = this->GetMaxChildIndex(2 * parent + 1);
    size_t child2 = this->GetMaxChildIndex(2 * parent + 2);
    return std::max(child1, child2);
  }

  // temporary vector uses by ComputeSizes() and allocated by
  // UpdateViewPositions().
  std::vector<int> Sizes;

  const int* ComputeSizes(int root, const int spacing)
  {
    assert(2 * root + 1 < static_cast<int>(this->Sizes.size()));

    const Cell& cell = this->KDTree[root];
    if (cell.Direction == vtkSMViewLayoutProxy::NONE)
    {
      int size[2] = { 0, 0 };
      if (cell.ViewProxy)
      {
        vtkSMPropertyHelper(cell.ViewProxy, "ViewSize").Get(size, 2);
      }
      this->Sizes[2 * root] = size[0];
      this->Sizes[2 * root + 1] = size[1];
      return &this->Sizes[2 * root];
    }

    const int* size0 = this->ComputeSizes(2 * root + 1, spacing);
    const int* size1 = this->ComputeSizes(2 * root + 2, spacing);

    // now double the width (or height) based on the split direction.
    if (cell.Direction == vtkSMViewLayoutProxy::HORIZONTAL)
    {
      this->Sizes[2 * root] = size0[0] + size1[0] + spacing;
      this->Sizes[2 * root + 1] = std::max(size0[1], size1[1]);
    }
    else
    {
      this->Sizes[2 * root] = std::max(size0[0], size1[0]);
      this->Sizes[2 * root + 1] = size0[1] + size1[1] + spacing;
    }
    return &this->Sizes[2 * root];
  }
};

//============================================================================
vtkStandardNewMacro(vtkSMViewLayoutProxy);
//----------------------------------------------------------------------------
vtkSMViewLayoutProxy::vtkSMViewLayoutProxy()
  : MaximizedCell(-1)
  , Internals(new vtkInternals())
  , BlockUpdate(false)
  , BlockUpdateViewPositions(false)
{
  this->Internals->Observer =
    vtkMakeMemberFunctionCommand(*this, &vtkSMViewLayoutProxy::UpdateViewPositions);

  // Push the root element.
  this->Internals->KDTree.resize(1);
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy::~vtkSMViewLayoutProxy()
{
  vtkMemberFunctionCommand<vtkSMViewLayoutProxy>::SafeDownCast(this->Internals->Observer)->Reset();
  this->Internals->Observer->Delete();
  this->Internals->Observer = nullptr;
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::Reset()
{
  this->Internals->KDTree.clear();
  this->Internals->KDTree.resize(1);
  this->UpdateState();
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::LoadState(const vtkSMMessage* message, vtkSMProxyLocator* locator)
{
  this->Superclass::LoadState(message, locator);

  if (message->ExtensionSize(ProxyState::user_data) != 1)
  {
    // vtkWarningMacro("Missing ViewLayoutState");
    return;
  }

  const ProxyState_UserData& user_data = message->GetExtension(ProxyState::user_data, 0);
  if (user_data.key() != "ViewLayoutState")
  {
    // vtkWarningMacro("Unexpected user_data. Expecting ViewLayoutState.");
    return;
  }

  const bool prev = this->SetBlockUpdateViewPositions(true);

  this->Internals->KDTree.clear();
  this->Internals->KDTree.resize(user_data.variant_size());

  for (int cc = 0; cc < user_data.variant_size(); cc++)
  {
    vtkInternals::Cell& cell = this->Internals->KDTree[cc];
    const Variant& value = user_data.variant(cc);

    cell.SplitFraction = value.float64(0);

    switch (value.integer(0))
    {
      case HORIZONTAL:
        cell.Direction = HORIZONTAL;
        break;

      case VERTICAL:
        cell.Direction = VERTICAL;
        break;

      case NONE:
      default:
        cell.Direction = NONE;
    }

    if (locator && vtkSMProxyProperty::CanCreateProxy())
    {
      cell.ViewProxy = vtkSMViewProxy::SafeDownCast(locator->LocateProxy(value.proxy_global_id(0)));
    }
    else
    {
      cell.ViewProxy =
        vtkSMViewProxy::SafeDownCast(this->GetSession()->GetRemoteObject(value.proxy_global_id(0)));
    }
    if (cell.ViewProxy && cell.ViewProxy->GetProperty("ViewSize"))
    {
      // every time view-size changes, we update the view positions for all views.
      cell.ViewProxy->GetProperty("ViewSize")
        ->AddObserver(vtkCommand::ModifiedEvent, this->Internals->Observer);
    }
  }

  this->SetBlockUpdateViewPositions(prev);
  this->UpdateViewPositions();

  // let the world know that the layout has been reconfigured.
  this->InvokeEvent(vtkCommand::ConfigureEvent);
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::UpdateState()
{
  if (this->BlockUpdate)
  {
    return;
  }

  // ensure that the state is created correctly.
  this->CreateVTKObjects();

  // push current state.
  this->State->ClearExtension(ProxyState::user_data);

  ProxyState_UserData* user_data = this->State->AddExtension(ProxyState::user_data);
  user_data->set_key("ViewLayoutState");

  for (size_t cc = 0; cc < this->Internals->KDTree.size(); cc++)
  {
    const vtkInternals::Cell& cell = this->Internals->KDTree[cc];

    Variant* variant = user_data->add_variant();
    variant->set_type(Variant::INT); // type is just arbitrary here.
    variant->add_integer(cell.Direction);
    variant->add_float64(cell.SplitFraction);
    variant->add_proxy_global_id(cell.ViewProxy ? cell.ViewProxy->GetGlobalID() : 0);
  }

  this->PushState(this->State);
  this->UpdateViewPositions();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "RemoveAllViews"
         << vtkClientServerStream::End;

  for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
       iter != this->Internals->KDTree.end(); ++iter)
  {
    if (iter->ViewProxy != nullptr &&
      vtkSMComparativeViewProxy::SafeDownCast(iter->ViewProxy) == nullptr)
    {
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "AddView"
             << VTKOBJECT(iter->ViewProxy)
             << vtkClientServerStream::InsertArray(iter->Viewport.GetData(), 4)
             << vtkClientServerStream::End;
    }
  }
  this->ExecuteStream(stream);

  this->InvokeEvent(vtkCommand::ConfigureEvent);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMViewLayoutProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator* iter)
{
  vtkPVXMLElement* element = this->Superclass::SaveXMLState(root, iter);
  if (!element)
  {
    return nullptr;
  }

  vtkPVXMLElement* layout = vtkPVXMLElement::New();
  layout->SetName("Layout");
  layout->AddAttribute("number_of_elements", static_cast<int>(this->Internals->KDTree.size()));
  element->AddNestedElement(layout);
  layout->Delete();

  for (size_t cc = 0; cc < this->Internals->KDTree.size(); cc++)
  {
    const vtkInternals::Cell& cell = this->Internals->KDTree[cc];

    vtkPVXMLElement* item = vtkPVXMLElement::New();
    item->SetName("Item");
    item->AddAttribute("direction", static_cast<int>(cell.Direction));
    item->AddAttribute("fraction", cell.SplitFraction);
    item->AddAttribute(
      "view", (cell.ViewProxy ? static_cast<unsigned int>(cell.ViewProxy->GetGlobalID()) : 0));
    layout->AddNestedElement(item);
    item->Delete();
  }

  return element;
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::LoadXMLState(element, locator))
  {
    return 0;
  }

  if (!locator)
  {
    return 1;
  }

  vtkPVXMLElement* layout = element->FindNestedElementByName("Layout");
  int number_of_elements = 0;
  if (!layout->GetScalarAttribute("number_of_elements", &number_of_elements) ||
    (number_of_elements <= 0))
  {
    vtkErrorMacro("Missing (or invalid) 'number_of_elements' attribute.");
    return 0;
  }

  if (static_cast<int>(layout->GetNumberOfNestedElements()) != number_of_elements)
  {
    vtkErrorMacro("Mismatch in number_of_elements and nested elements.");
    return 0;
  }

  this->Internals->KDTree.clear();
  this->Internals->KDTree.resize(number_of_elements);
  for (unsigned int cc = 0; cc < layout->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* item = layout->GetNestedElement(cc);
    if (item == nullptr || item->GetName() == nullptr || strcmp(item->GetName(), "Item") != 0)
    {
      vtkErrorMacro("Invalid nested element at index : " << cc);
      return 0;
    }
    int direction, viewid;
    double fraction;
    if (!item->GetScalarAttribute("direction", &direction) ||
      !item->GetScalarAttribute("fraction", &fraction) ||
      !item->GetScalarAttribute("view", &viewid))
    {
      vtkErrorMacro("Invalid nested element at index : " << cc);
      return 0;
    }
    vtkInternals::Cell& cell = this->Internals->KDTree[cc];
    cell.Direction =
      ((direction == NONE) ? NONE : ((direction == HORIZONTAL) ? HORIZONTAL : VERTICAL));
    cell.SplitFraction = fraction;
    if (viewid)
    {
      cell.ViewProxy = vtkSMViewProxy::SafeDownCast(locator->LocateProxy(viewid));
    }
    else
    {
      cell.ViewProxy = nullptr;
    }
    if (cell.ViewProxy && cell.ViewProxy->GetProperty("ViewSize"))
    {
      // every time view-size changes, we update the view positions for all views.
      cell.ViewProxy->GetProperty("ViewSize")
        ->AddObserver(vtkCommand::ModifiedEvent, this->Internals->Observer);
    }
  }

  this->UpdateViewPositions();
  this->UpdateState();
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::Split(int location, int direction, double fraction)
{
  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return 0;
  }

  // we don't get by value, since we resize the vector.
  vtkInternals::Cell cell = this->Internals->KDTree[location];
  if (cell.Direction != NONE)
  {
    vtkErrorMacro("Cell identified by location '"
      << location << "' is already split. Cannot split the cell again.");
    return 0;
  }

  if (direction <= NONE || direction > HORIZONTAL)
  {
    vtkErrorMacro("Invalid direction : " << direction);
    return 0;
  }

  if (fraction < 0.0 || fraction > 1.0)
  {
    vtkErrorMacro("Invalid fraction : " << fraction << ". Must be in the range [0, 1]");
    return 0;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg(direction == VERTICAL ? "SplitVertical" : "SplitHorizontal")
    .arg(location)
    .arg(fraction)
    .arg("comment", "split cell");

  cell.Direction = (direction == VERTICAL) ? VERTICAL : HORIZONTAL;
  cell.SplitFraction = fraction;

  // ensure that both the children (2i+1), (2i+2) can fit in the KDTree.
  if ((2 * location + 2) >= static_cast<int>(this->Internals->KDTree.size()))
  {
    this->Internals->KDTree.resize(2 * location + 2 + 1);
  }

  int child_location = (2 * location + 1);
  if (cell.ViewProxy)
  {
    vtkInternals::Cell& child = this->Internals->KDTree[child_location];
    child.ViewProxy = cell.ViewProxy;
    cell.ViewProxy = nullptr;
  }
  this->Internals->KDTree[location] = cell;
  this->MaximizedCell = -1;
  this->UpdateState();
  return child_location;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::AssignView(int location, vtkSMViewProxy* view)
{
  if (view == nullptr)
  {
    return false;
  }

  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
  }

  vtkInternals::Cell& cell = this->Internals->KDTree[location];
  if (cell.Direction != NONE)
  {
    vtkErrorMacro("Cell is not a leaf '" << location << "'."
                                                        " Cannot assign a view to it.");
    return false;
  }

  if (cell.ViewProxy != nullptr && cell.ViewProxy != view)
  {
    vtkErrorMacro("Cell is not empty.");
    return false;
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("AssignViewToLayout")
    .arg("view", view)
    .arg("layout", this)
    .arg("hint", location)
    .arg("comment", "assign view to a particular cell in the layout");

  if (cell.ViewProxy == view)
  {
    // nothing to do.
    return true;
  }

  cell.ViewProxy = view;

  if (view->GetProperty("ViewSize"))
  {
    // every time view-size changes, we update the view positions for all views.
    view->GetProperty("ViewSize")
      ->AddObserver(vtkCommand::ModifiedEvent, this->Internals->Observer);
  }

  this->UpdateState();
  return true;
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::AssignViewToAnyCell(vtkSMViewProxy* view, int location_hint)
{
  if (!view)
  {
    return 0;
  }

  int cur_location = this->GetViewLocation(view);
  if (cur_location != -1)
  {
    // already assigned to a frame. return that index.
    SM_SCOPED_TRACE(CallMethod)
      .arg(this)
      .arg("AssignView")
      .arg(cur_location)
      .arg(view)
      .arg("comment", "place view in the layout");
    return cur_location;
  }

  if (location_hint < 0)
  {
    location_hint = 0;
  }

  // If location_hint refers to an empty cell, use it.
  if (this->Internals->IsCellValid(location_hint))
  {
    int empty_cell = this->GetEmptyCell(location_hint);
    if (empty_cell >= 0)
    {
      return this->AssignView(empty_cell, view);
    }
  }
  else
  {
    // make location_hint a valid location.
    location_hint = 0;
  }

  // Find any empty cell.
  int empty_cell = this->GetEmptyCell(0);
  if (empty_cell >= 0)
  {
    return this->AssignView(empty_cell, view);
  }

  // No empty cell found, split a view, starting with the location_hint.
  Direction direction = HORIZONTAL;

  int parent = this->GetParent(location_hint);
  if (parent >= 0)
  {
    direction = this->GetSplitDirection(parent) == HORIZONTAL ? VERTICAL : HORIZONTAL;
  }
  int split_cell = this->GetSplittableCell(location_hint, direction);
  assert(split_cell >= 0);

  bool prev = this->SetBlockUpdate(true);
  int new_cell = this->Split(split_cell, direction, 0.5);
  this->SetBlockUpdate(prev);

  if (this->GetView(new_cell) == nullptr)
  {
    return this->AssignView(new_cell, view);
  }

  return this->AssignView(new_cell + 1, view);
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::RemoveView(int index)
{
  return this->RemoveView(this->GetView(index)) != -1;
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::RemoveView(vtkSMViewProxy* view)
{
  if (!view)
  {
    return -1;
  }

  int index = 0;
  for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
       iter != this->Internals->KDTree.end(); ++iter, ++index)
  {
    if (iter->ViewProxy == view)
    {
      if (iter->ViewProxy->GetProperty("ViewSize"))
      {
        iter->ViewProxy->GetProperty("ViewSize")->RemoveObserver(this->Internals->Observer);
      }
      iter->ViewProxy = nullptr;
      this->UpdateState();
      return index;
    }
  }

  return -1;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::SwapCells(int location1, int location2)
{
  if (!this->Internals->IsCellValid(location1) || !this->Internals->IsCellValid(location2))
  {
    vtkErrorMacro("Invalid locations specified.");
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("SwapCells")
    .arg(location1)
    .arg(location2)
    .arg("comment", "swap view locations");

  vtkInternals::Cell& cell1 = this->Internals->KDTree[location1];
  vtkInternals::Cell& cell2 = this->Internals->KDTree[location2];
  if (cell1.Direction == NONE && cell2.Direction == NONE)
  {
    vtkSMViewProxy* temp1 = cell1.ViewProxy;
    cell1.ViewProxy = cell2.ViewProxy;
    cell2.ViewProxy = temp1;
    this->UpdateState();
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::Collapse(int location)
{
  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
  }

  vtkInternals::Cell& cell = this->Internals->KDTree[location];
  if (cell.Direction != NONE)
  {
    vtkErrorMacro("Only leaf cells can be collapsed.");
    return false;
  }

  if (cell.ViewProxy != nullptr)
  {
    vtkErrorMacro("Only empty cells can be collapsed.");
    return false;
  }

  if (location == 0)
  {
    // sure, trying to collapse the root node...whatever!!!
    return true;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("Collapse")
    .arg(location)
    .arg("comment", "close an empty frame");

  int parent = (location - 1) / 2;
  int sibling = ((location % 2) == 0) ? (2 * parent + 1) : (2 * parent + 2);

  this->Internals->MoveSubtree(parent, sibling);
  this->Internals->Shrink();
  this->MaximizedCell = -1;
  this->UpdateState();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::IsSplitCell(int location)
{
  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
  }

  const vtkInternals::Cell& cell = this->Internals->KDTree[location];
  return (cell.Direction != NONE);
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::GetEmptyCell(int root)
{
  vtkInternals::Cell& cell = this->Internals->KDTree[root];
  if (cell.Direction == NONE && cell.ViewProxy == nullptr)
  {
    return root;
  }
  else if (cell.Direction == HORIZONTAL || cell.Direction == VERTICAL)
  {
    int child0 = 2 * root + 1;
    int empty_cell = this->GetEmptyCell(child0);
    if (empty_cell >= 0)
    {
      return empty_cell;
    }
    int child1 = 2 * root + 2;
    empty_cell = this->GetEmptyCell(child1);
    if (empty_cell >= 0)
    {
      return empty_cell;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::GetSplittableCell(
  int root, vtkSMViewLayoutProxy::Direction& suggested_direction)
{
  vtkInternals::Cell& cell = this->Internals->KDTree[root];
  if (cell.Direction == NONE)
  {
    return root;
  }
  else if (cell.Direction == HORIZONTAL || cell.Direction == VERTICAL)
  {
    suggested_direction = cell.Direction == HORIZONTAL ? VERTICAL : HORIZONTAL;
    int child0 = 2 * root + 1;
    return this->GetSplittableCell(child0, suggested_direction);
  }
  return -1;
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy::Direction vtkSMViewLayoutProxy::GetSplitDirection(int location)
{
  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return NONE;
  }

  return this->Internals->KDTree[location].Direction;
}

//----------------------------------------------------------------------------
double vtkSMViewLayoutProxy::GetSplitFraction(int location)
{
  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return NONE;
  }

  return this->Internals->KDTree[location].SplitFraction;
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMViewLayoutProxy::GetView(int location)
{
  if (!this->Internals->IsCellValid(location))
  {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return nullptr;
  }

  return this->Internals->KDTree[location].ViewProxy;
}

//----------------------------------------------------------------------------
int vtkSMViewLayoutProxy::GetViewLocation(vtkSMViewProxy* view)
{
  int index = 0;
  for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
       iter != this->Internals->KDTree.end(); ++iter, ++index)
  {
    if (iter->ViewProxy == view)
    {
      return index;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::SetSplitFraction(int location, double val)
{
  if (val < 0.0 || val > 1.0)
  {
    vtkErrorMacro("Invalid fraction : " << val << ". Must be in the range [0, 1]");
    return 0;
  }

  if (!this->IsSplitCell(location))
  {
    return false;
  }

  SM_SCOPED_TRACE(CallMethod)
    .arg(this)
    .arg("SetSplitFraction")
    .arg(location)
    .arg(val)
    .arg("comment", "resize frame");

  if (this->Internals->KDTree[location].SplitFraction != val)
  {
    this->Internals->KDTree[location].SplitFraction = val;
    this->MaximizedCell = -1;
    this->UpdateState();
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::UpdateViewPositions()
{
  if (this->BlockUpdateViewPositions)
  {
    return;
  }

  if (this->MaximizedCell == -1)
  {
    auto vlayout = vtkViewLayout::SafeDownCast(this->GetClientSideObject());
    assert(vlayout);

    this->Internals->UpdateViewPositions(vlayout->GetSeparatorWidth());
    this->Internals->UpdateViewports();
  }
  else
  {
    // simply set all ViewPositions to 0.
    for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
         iter != this->Internals->KDTree.end(); ++iter)
    {
      if (iter->ViewProxy.GetPointer() != nullptr)
      {
        int pos[2] = { 0, 0 };
        vtkSMPropertyHelper(iter->ViewProxy, "ViewPosition").Set(pos, 2);
        iter->ViewProxy->UpdateProperty("ViewPosition");
        iter->Viewport = vtkVector4d(0, 0, 1, 1);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::GetLayoutExtent(int extent[4])
{
  vtkRecti rect(0, 0, 0, 0);
  bool first = true;
  for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
       iter != this->Internals->KDTree.end(); ++iter)
  {
    if (iter->ViewProxy.GetPointer() != nullptr)
    {
      int vpos[2] = { 0, 0 };
      vtkSMPropertyHelper(iter->ViewProxy, "ViewPosition").Get(vpos, 2);

      int vsize[2] = { 0, 0 };
      vtkSMPropertyHelper(iter->ViewProxy, "ViewSize").Get(vsize, 2);

      if (first)
      {
        rect = vtkRecti(vpos[0], vpos[1], vsize[0], vsize[1]);
        first = false;
      }
      else
      {
        rect.AddRect(vtkRecti(vpos[0], vpos[1], vsize[0], vsize[1]));
      }
    }
  }
  extent[0] = rect.GetLeft();
  extent[1] = rect.GetRight() - 1;
  extent[2] = rect.GetBottom();
  extent[3] = rect.GetTop() - 1;
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::SetSize(const int size[2])
{
  const bool prev = this->SetBlockUpdateViewPositions(true);

  // recursively distribute space available among views.
  auto vlayout = vtkViewLayout::SafeDownCast(this->GetClientSideObject());
  assert(vlayout);
  this->Internals->ResizeCell(vlayout->GetSeparatorWidth(), 0, vtkVector2i(size));
  this->SetBlockUpdateViewPositions(prev);

  this->UpdateViewPositions();
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::ShowViewsOnTileDisplay()
{
  this->CreateVTKObjects();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ShowOnTileDisplay"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::MaximizeCell(int location)
{
  if (this->Internals->IsCellValid(location) == false || this->IsSplitCell(location))
  {
    return false;
  }

  this->MaximizedCell = location;
  this->UpdateState();
  return true;
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::RestoreMaximizedState()
{
  if (this->MaximizedCell != -1)
  {
    this->MaximizedCell = -1;
    this->UpdateState();
  }
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMViewLayoutProxy::CaptureWindow(int magX, int magY)
{
  if (this->MaximizedCell != -1)
  {
    vtkSMViewProxy* view = this->GetView(this->MaximizedCell);
    if (view)
    {
      return view->CaptureWindow(magX, magY);
    }
    vtkErrorMacro("No view present in the layout.");
    return nullptr;
  }

  std::vector<vtkSmartPointer<vtkImageData> > images;
  for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
       iter != this->Internals->KDTree.end(); ++iter)
  {
    if (iter->ViewProxy.GetPointer())
    {
      vtkImageData* image = iter->ViewProxy->CaptureWindow(magX, magY);
      if (image)
      {
        images.push_back(image);
        image->FastDelete();
      }
    }
  }

  if (images.size() == 0)
  {
    vtkErrorMacro("No view present in the layout.");
    return nullptr;
  }

  auto vlayout = vtkViewLayout::SafeDownCast(this->GetClientSideObject());
  assert(vlayout);

  double dcolor[3];
  vlayout->GetSeparatorColor(dcolor);

  unsigned char uccolor[3] = { static_cast<unsigned char>(255 * dcolor[0]),
    static_cast<unsigned char>(255 * dcolor[1]), static_cast<unsigned char>(255 * dcolor[2]) };

  // note, we pass separator width as 0 to MergeImages since the individual
  // images already have spacing/separator width taken into consideration.
  auto img = vtkSMUtilities::MergeImages(images, 0, uccolor);
  if (img)
  {
    img->Register(this);
  }
  return img.GetPointer();
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy* vtkSMViewLayoutProxy::FindLayout(
  vtkSMViewProxy* view, const char* reggroup /*=layouts*/)
{
  if (!view)
  {
    return nullptr;
  }
  vtkSMSessionProxyManager* pxm = view->GetSessionProxyManager();
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();
  for (iter->Begin(reggroup); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMViewLayoutProxy* layout = vtkSMViewLayoutProxy::SafeDownCast(iter->GetProxy());
    if (layout != nullptr && layout->GetViewLocation(view) != -1)
    {
      return layout;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::ContainsView(vtkSMProxy* proxy)
{
  if (vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(proxy))
  {
    return this->ContainsView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
std::vector<vtkSMViewProxy*> vtkSMViewLayoutProxy::GetViews()
{
  std::vector<vtkSMViewProxy*> views;
  for (vtkInternals::KDTreeType::iterator iter = this->Internals->KDTree.begin();
       iter != this->Internals->KDTree.end(); ++iter)
  {
    if (iter->ViewProxy.GetPointer())
    {
      views.push_back(iter->ViewProxy);
    }
  }
  return views;
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::SaveAsPNG(int rank, const char* fname)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SaveAsPNG" << rank << fname
         << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER);
  // ensure that the server is done saving the image before proceeding.
  this->GetLastResult(vtkPVSession::RENDER_SERVER_ROOT);
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
