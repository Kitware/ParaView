#include "vtkPiece.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPiece, "1.1");
vtkStandardNewMacro(vtkPiece);

//----------------------------------------------------------------------------
vtkPiece::vtkPiece()
{
  this->Piece = 0;
  this->NumPieces = 1;
  this->Priority = 1.0;
  this->Resolution = 1.0;
}

//----------------------------------------------------------------------------
vtkPiece::~vtkPiece()
{
}

//----------------------------------------------------------------------------
void vtkPiece::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPiece::CopyPiece(vtkPiece *other)
{
  if (!other)
    {
    cerr << "Warning attempt to copy from NULL vtkPiece" << endl;
    return;
    }
  this->SetPiece(other->GetPiece());
  this->SetNumPieces(other->GetNumPieces());
  this->SetPriority(other->GetPriority());
  this->SetResolution(other->GetResolution());
}

//----------------------------------------------------------------------------
void vtkPiece::Serialize(double *buff, double **optr)
{
  double *ptr = buff;
  if (!buff || !optr)
    {
    return;
    }
  *ptr = (double)this->Piece;
  ptr++;
  *ptr = (double)this->NumPieces;
  ptr++;
  *ptr = this->Priority;
  ptr++;
  *ptr = this->Resolution;
  ptr++;

  *optr = ptr;
}

//----------------------------------------------------------------------------
void vtkPiece::UnSerialize(double *buff, double **optr)
{
  double *ptr = buff;
  if (!buff || !optr)
    {
    return;
    }
  this->Piece = (int)*ptr;
  ptr++;
  this->NumPieces = (int)*ptr;
  ptr++;
  this->Priority = *ptr;
  ptr++;
  this->Resolution = *ptr;
  ptr++;

  *optr = ptr;
}

