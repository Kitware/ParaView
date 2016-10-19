/*=========================================================================

  Program:   ParaView
  Module:    vtkKdTreeGenerator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKdTreeGenerator.h"

#include "vtkBSPCuts.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkKdNode.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

class vtkKdTreeGeneratorVector : public std::vector<int>
{
};

vtkStandardNewMacro(vtkKdTreeGenerator);
vtkCxxSetObjectMacro(vtkKdTreeGenerator, ExtentTranslator, vtkExtentTranslator);
vtkCxxSetObjectMacro(vtkKdTreeGenerator, KdTree, vtkPKdTree);
//-----------------------------------------------------------------------------
vtkKdTreeGenerator::vtkKdTreeGenerator()
{
  this->NumberOfPieces = 1;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 1;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;
  this->ExtentTranslator = 0;
  this->Regions = 0;
  this->KdTree = 0;
}

//-----------------------------------------------------------------------------
vtkKdTreeGenerator::~vtkKdTreeGenerator()
{
  this->SetKdTree(0);
  this->SetExtentTranslator(0);
  delete[] this->Regions;
  this->Regions = 0;
}

//-----------------------------------------------------------------------------
void vtkKdTreeGeneratorOrder(int*& ptr, vtkKdNode* node)
{
  if (node->GetLeft())
  {
    vtkKdTreeGeneratorOrder(ptr, node->GetLeft());
    vtkKdTreeGeneratorOrder(ptr, node->GetRight());
  }
  else
  {
    *ptr = node->GetID();
    ptr++;
  }
}

//-----------------------------------------------------------------------------
bool vtkKdTreeGenerator::BuildTree(vtkExtentTranslator* translator, const int extents[6],
  const double origin[3], const double spacing[3])
{
  if (!translator)
  {
    vtkErrorMacro("Cannot generate k-d tree without any ExtentTranslator.");
    return false;
  }

  if (extents[0] > extents[1] || extents[2] > extents[3] || extents[4] > extents[5])
  {
    vtkErrorMacro("Invalid extents. Cannot generate KdTree.");
    return false;
  }

  if (spacing[0] < 0 || spacing[1] < 0 || spacing[2] < 0)
  {
    vtkErrorMacro("Invalid spacing. Cannot generate KdTree.");
    return false;
  }

  this->SetExtentTranslator(translator);
  this->SetWholeExtent(const_cast<int*>(extents));
  this->SetOrigin(const_cast<double*>(origin));
  this->SetSpacing(const_cast<double*>(spacing));

  vtkSmartPointer<vtkKdNode> root = vtkSmartPointer<vtkKdNode>::New();
  root->DeleteChildNodes();
  root->SetBounds(this->WholeExtent[0], this->WholeExtent[1], this->WholeExtent[2],
    this->WholeExtent[3], this->WholeExtent[4], this->WholeExtent[5]);
  root->SetDim(0);

  this->FormRegions();
  vtkKdTreeGeneratorVector regions_ids;
  for (int cc = 0; cc < this->NumberOfPieces; cc++)
  {
    regions_ids.push_back(cc);
  }
  if (!this->FormTree(root, regions_ids))
  {
    return 0;
  }

  // The formed tree is based on extents, we convert it to bounds.
  if (!this->ConvertToBounds(root))
  {
    return 0;
  }

  // Now to determine assigments (not much different from inorder traversal printing
  // the leaf nodes alone).
  int* assignments = new int[this->NumberOfPieces];
  int* ptr = assignments;
  // assignments[0] = 0;
  // assignments[1] = 1;
  // assignments[2] = 2;
  // assignments[3] = 3;
  vtkKdTreeGeneratorOrder(ptr, root);
  this->KdTree->AssignRegions(assignments, this->NumberOfPieces);

  vtkSmartPointer<vtkBSPCuts> cuts = vtkSmartPointer<vtkBSPCuts>::New();
  cuts->CreateCuts(root);
  if (!this->KdTree)
  {
    vtkPKdTree* tree = vtkPKdTree::New();
    this->SetKdTree(tree);
    tree->Delete();
  }
  this->KdTree->SetCuts(cuts);
  // cout  << endl << "Tree: " << endl;
  // cuts->PrintTree();

  this->SetExtentTranslator(0);
  delete[] assignments;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkKdTreeGenerator::FormTree(vtkKdNode* parent, vtkKdTreeGeneratorVector& regions_ids)
{
  if (regions_ids.size() == 1)
  {
    // We set the ID here so that it helps us figure out the
    // region assignments, KdTree will replace these IDs
    // when it reorders the regions.
    parent->SetID(regions_ids[0]);
    parent->SetDim(3);
    int* extent = &this->Regions[6 * regions_ids[0]];
    parent->SetBounds(extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
    return 1;
  }
  if (regions_ids.size() == 0)
  {
    vtkErrorMacro("RegionIDs cannot be 0.");
    return 0;
  }

  // Now we must determine how the given regions can be split into
  // two non-interacting partitions.
  int start_dim = parent->GetDim();
  int current_dim = start_dim;
  if (start_dim == 3)
  {
    vtkErrorMacro("Cannot partition leaf node!");
    return 0;
  }

  vtkKdTreeGeneratorVector left;
  vtkKdTreeGeneratorVector right;
  int division_point = 0;
  do
  {
    for (unsigned int cc = 0; cc < regions_ids.size(); cc++)
    {
      int region_id = regions_ids[cc];
      int* region_extents = &this->Regions[6 * region_id];

      division_point = region_extents[current_dim * 2 + 1];
      if (this->CanPartition(division_point, current_dim, regions_ids, left, right))
      {
        break;
      }
    }
    if (left.size() > 0 || right.size() > 0)
    {
      break;
    }
    current_dim = (current_dim + 1) % 3;
  } while (current_dim != start_dim);

  parent->SetDim(current_dim);

  vtkKdNode* leftNode = vtkKdNode::New();
  leftNode->SetDim((current_dim + 1) % 3);
  double bounds[6];
  parent->GetBounds(bounds);
  bounds[2 * current_dim + 1] = division_point;
  leftNode->SetBounds(bounds);

  if (!this->FormTree(leftNode, left))
  {
    leftNode->Delete();
    return 0;
  }
  parent->SetLeft(leftNode);
  leftNode->Delete();

  vtkKdNode* rightNode = vtkKdNode::New();
  rightNode->SetDim((current_dim + 1) % 3);
  parent->GetBounds(bounds);
  bounds[2 * current_dim] = division_point;
  rightNode->SetBounds(bounds);
  if (!this->FormTree(rightNode, right))
  {
    rightNode->Delete();
    return 0;
  }
  parent->SetRight(rightNode);
  rightNode->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkKdTreeGenerator::CanPartition(int division_point, int dimension,
  vtkKdTreeGeneratorVector& ids, vtkKdTreeGeneratorVector& left, vtkKdTreeGeneratorVector& right)
{
  // Iterate over all regions in the ids list and see if the given
  // division point is inside any region. If so, return false,
  // otherwise true.
  // On success we update left, right to reflect the regions to the left
  // and right of the division_point.

  vtkKdTreeGeneratorVector work_left;
  vtkKdTreeGeneratorVector work_right;

  for (unsigned int cc = 0; cc < ids.size(); cc++)
  {
    int region_id = ids[cc];
    int* region_extents = &this->Regions[6 * region_id];
    int min = region_extents[2 * dimension];
    int max = region_extents[2 * dimension + 1];
    if (division_point > min && division_point < max)
    {
      // division_point intersects some region.
      // division is not valid.
      return 0;
    }
    if (division_point <= min)
    {
      work_right.push_back(region_id);
    }
    else // if (division_point >= max)
    {
      work_left.push_back(region_id);
    }
  }
  if (work_right.size() == 0 || work_left.size() == 0)
  {
    return 0;
  }
  left = work_left;
  right = work_right;
  return 1;
}

//-----------------------------------------------------------------------------
void vtkKdTreeGenerator::FormRegions()
{
  delete[] this->Regions;
  this->Regions = new int[this->NumberOfPieces * 6];
  this->ExtentTranslator->SetWholeExtent(this->WholeExtent);
  this->ExtentTranslator->SetNumberOfPieces(this->NumberOfPieces);
  this->ExtentTranslator->SetGhostLevel(0);
  // cout << "************************" << endl;
  // cout << "ExtentTranslator: " << this->ExtentTranslator << ":"
  //   << this->ExtentTranslator->GetClassName() << endl;
  for (int cc = 0; cc < this->NumberOfPieces; cc++)
  {
    this->ExtentTranslator->SetPiece(cc);
    this->ExtentTranslator->PieceToExtent();
    this->ExtentTranslator->GetExtent(&this->Regions[cc * 6]);
    // int extent[6];
    // this->ExtentTranslator->GetExtent(extent);
    // cout << cc << ": "
    //  << extent[0] << ", "
    //  << extent[1] << ", "
    //  << extent[2] << ", "
    //  << extent[3] << ", "
    //  << extent[4] << ", "
    //  << extent[5] << endl;
  }
}

//-----------------------------------------------------------------------------
static bool vtkConvertToBoundsInternal(vtkKdNode* node, double origin[3], double spacing[3])
{
  // convert extents to bounds (copied from vtkImageData::ConvertToBounds())
  double extent[6];
  node->GetBounds(extent);

  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
  {
    return false;
  }

  int swapXBounds = (spacing[0] < 0); // 1 if true, 0 if false
  int swapYBounds = (spacing[1] < 0); // 1 if true, 0 if false
  int swapZBounds = (spacing[2] < 0); // 1 if true, 0 if false

  double bounds[6];
  bounds[0] = origin[0] + (extent[0 + swapXBounds] * spacing[0]);
  bounds[2] = origin[1] + (extent[2 + swapYBounds] * spacing[1]);
  bounds[4] = origin[2] + (extent[4 + swapZBounds] * spacing[2]);

  bounds[1] = origin[0] + (extent[1 - swapXBounds] * spacing[0]);
  bounds[3] = origin[1] + (extent[3 - swapYBounds] * spacing[1]);
  bounds[5] = origin[2] + (extent[5 - swapZBounds] * spacing[2]);
  node->SetBounds(bounds);

  if (node->GetLeft())
  {
    if (!vtkConvertToBoundsInternal(node->GetLeft(), origin, spacing))
    {
      return false;
    }
  }
  if (node->GetRight())
  {
    if (!vtkConvertToBoundsInternal(node->GetRight(), origin, spacing))
    {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkKdTreeGenerator::ConvertToBounds(vtkKdNode* node)
{
  return vtkConvertToBoundsInternal(node, this->Origin, this->Spacing);
}

//-----------------------------------------------------------------------------
void vtkKdTreeGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "KdTree: " << this->KdTree << endl;
}
