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

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkWeakPointer.h"

#include <vector>
#include <assert.h>

class vtkSMViewLayoutProxy::vtkInternals
{
public:
  struct Cell
    {
    vtkSMViewLayoutProxy::Direction Direction;
    double SplitFraction;
    vtkWeakPointer<vtkSMProxy> ViewProxy;

    Cell() :
      Direction(vtkSMViewLayoutProxy::NONE),
      SplitFraction(0.5)
    {
    }
    };


  bool IsCellValid(size_t location)
    {
    if (location >= this->KDTree.size())
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

  void MoveSubtree(unsigned int destination, unsigned int source)
    {
    // we only support moving a subtree "up".
    assert(destination < source);

    if (source >= this->KDTree.size() || destination >= this->KDTree.size())
      {
      return;
      }
      
    Cell source_cell = this->KDTree[source];
    this->MoveSubtree(2*destination+1, 2*source+1);
    this->MoveSubtree(2*destination+2, 2*source+2);
    this->KDTree[destination] = source_cell;
    }

  void Shrink()
    {
    size_t max_index = this->GetMaxChildIndex(0);
    assert(max_index < this->KDTree.size());
    this->KDTree.resize(max_index + 1);
    }

  typedef std::vector<Cell> KDTreeType;
  KDTreeType KDTree;

private:
  size_t GetMaxChildIndex(size_t parent)
    {
    size_t child0 = 2*parent + 1;
    size_t child1 = 2*parent + 2;
    if (child1 < this->KDTree.size())
      {
      return this->GetMaxChildIndex(child1);
      }
    else if (child0 < this->KDTree.size())
      {
      return this->GetMaxChildIndex(child0);
      }
    return parent;
    }
};

vtkStandardNewMacro(vtkSMViewLayoutProxy);
//----------------------------------------------------------------------------
vtkSMViewLayoutProxy::vtkSMViewLayoutProxy() :
  Internals(new vtkInternals())
{
  // Push the root element.
  this->Internals->KDTree.resize(1);
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy::~vtkSMViewLayoutProxy()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
unsigned int vtkSMViewLayoutProxy::Split(
  unsigned int location, int direction, double fraction)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return 0;
    }

  // we don't get by value, since we resize the vector.
  vtkInternals::Cell cell = this->Internals->KDTree[location];
  if (cell.Direction != NONE)
    {
    vtkErrorMacro("Cell identified by location '" << location
      << "' is already split. Cannot split the cell again.");
    return 0;
    }

  if (direction <= NONE || direction > HORIZONTAL)
    {
    vtkErrorMacro("Invalid direction : " << direction);
    return 0;
    }

  if (fraction < 0.0 || fraction > 1.0)
    {
    vtkErrorMacro("Invalid fraction : " << fraction
      << ". Must be in the range [0, 1]");
    return 0;
    }

  cell.Direction = (direction == VERTICAL)? VERTICAL : HORIZONTAL;
  cell.SplitFraction = fraction;

  // ensure that both the children (2i+1), (2i+2) can fit in the KDTree.
  if ( (2*location + 2) >= this->Internals->KDTree.size())
    {
    this->Internals->KDTree.resize(2*location + 2 + 1);
    }


  unsigned int child_location = (2*location + 1);
  if (cell.ViewProxy)
    {
    vtkInternals::Cell &child = this->Internals->KDTree[2*location + 1];
    child.ViewProxy = cell.ViewProxy;
    cell.ViewProxy = NULL;
    }
  this->Internals->KDTree[location] = cell;

  return child_location;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::AssignView(unsigned int location, vtkSMProxy* view)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
    }

  vtkInternals::Cell &cell = this->Internals->KDTree[location];
  if (cell.Direction != NONE)
    {
    vtkErrorMacro("Cell is not a leaf '" << location << "'."
      " Cannot assign a view to it.");
    return false;
    }

  if (cell.ViewProxy != NULL && cell.ViewProxy != view)
    {
    vtkErrorMacro("Cell is not empty.");
    return false;
    }

  cell.ViewProxy = view;
  return true;
}

//----------------------------------------------------------------------------
unsigned int vtkSMViewLayoutProxy::RemoveView(vtkSMProxy* view)
{
  unsigned int index = 0;
  for (vtkInternals::KDTreeType::iterator iter =
    this->Internals->KDTree.begin();
    iter != this->Internals->KDTree.end(); ++iter, ++index)
    {
    if (iter->ViewProxy == view)
      {
      iter->ViewProxy = NULL;
      return index;
      }
    }

  return static_cast<unsigned int>(-1);
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::Collape(unsigned int location)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
    }

  vtkInternals::Cell &cell = this->Internals->KDTree[location];
  if (cell.Direction != NONE)
    {
    vtkErrorMacro("Only leaf cells can be collapsed.");
    return false;
    }
  
  if (cell.ViewProxy != NULL)
    {
    vtkErrorMacro("Only empty cells can be collapsed.");
    return false;
    }

  if (location == 0)
    {
    // sure, trying to collapse the root node...whatever!!!
    return true;
    }

  unsigned int parent = (location -1) / 2;
  unsigned int sibling = ((location % 2) == 0)?
    (2*parent + 1) : (2*parent + 2);

  this->Internals->MoveSubtree(parent, sibling);
  this->Internals->Shrink();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::IsSplitCell(unsigned int location)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
    }

  const vtkInternals::Cell &cell = this->Internals->KDTree[location];
  return (cell.Direction != NONE);
}

//----------------------------------------------------------------------------
vtkSMViewLayoutProxy::Direction vtkSMViewLayoutProxy::GetSplitDirection(unsigned int location)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return NONE;
    }

  return this->Internals->KDTree[location].Direction;
}

//----------------------------------------------------------------------------
double vtkSMViewLayoutProxy::GetSplitFraction(unsigned int location)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return NONE;
    }

  return this->Internals->KDTree[location].SplitFraction;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMViewLayoutProxy::GetView(unsigned int location)
{
  if (!this->Internals->IsCellValid(static_cast<size_t>(location)))
    {
    vtkErrorMacro("Invalid location '" << location << "' specified.");
    return false;
    }

  return this->Internals->KDTree[location].ViewProxy;
}

//----------------------------------------------------------------------------
bool vtkSMViewLayoutProxy::SetSplitFraction(unsigned int location, double val)
{
  if (val < 0.0 || val > 1.0)
    {
    vtkErrorMacro("Invalid fraction : " << val 
      << ". Must be in the range [0, 1]");
    return 0;
    }

  if (!this->IsSplitCell(location))
    {
    return false;
    }

  this->Internals->KDTree[location].SplitFraction = val;
  return true;
}

//----------------------------------------------------------------------------
void vtkSMViewLayoutProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
