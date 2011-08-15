#include "vtkPiece.h"

#include <iostream>
using namespace std;

//----------------------------------------------------------------------------
vtkPiece::vtkPiece()
{
  this->Processor = 0;
  this->Piece = 0;
  this->NumPieces = 1;
  this->Resolution = 1.0;
  this->ReachedLimit = false;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -1;
  this->PipelinePriority = this->ViewPriority = this->CachedPriority = 1.0;
}

//----------------------------------------------------------------------------
vtkPiece::~vtkPiece()
{
}

//----------------------------------------------------------------------------
void vtkPiece::CopyPiece(vtkPiece other)
{
  if (!other.IsValid())
    {
    cerr << "Warning attempt to copy from NULL vtkPiece" << endl;
    return;
    }
  this->SetProcessor(other.GetProcessor());
  this->SetPiece(other.GetPiece());
  this->SetNumPieces(other.GetNumPieces());
  this->SetResolution(other.GetResolution());
  this->SetPipelinePriority(other.GetPipelinePriority());
  this->SetViewPriority(other.GetViewPriority());
  this->SetCachedPriority(other.GetCachedPriority());
  this->SetBounds(other.GetBounds());
  this->SetReachedLimit(other.GetReachedLimit());
}
