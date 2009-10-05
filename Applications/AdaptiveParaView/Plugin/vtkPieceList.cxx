#include "vtkPieceList.h"
#include "vtkObjectFactory.h"
#include "vtkPiece.h"

#include "vtkstd/vector"
#include "vtkstd/algorithm"

vtkCxxRevisionMacro(vtkPieceList, "1.1");
vtkStandardNewMacro(vtkPieceList);

///////////////////////////////////////////////////////////////////////////
class vtkInternals
{
public:
  vtkInternals()
  {
    this->SerializeBuffer = NULL;
    this->BufferSize = 0;
  }
  ~vtkInternals()
  {
  if (this->SerializeBuffer != NULL)
    {
    delete[] this->SerializeBuffer;
    }
  }
  vtkstd::vector<vtkPiece *> Pieces;
  double *SerializeBuffer;
  int BufferSize;
};

class vtkPieceListByPriority 
{
public:
  bool operator() (vtkPiece *one, vtkPiece *two)
  {
  return one->ComparePriority(two);
  }
};

///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
vtkPieceList::vtkPieceList()
{
  this->Internals = new vtkInternals;
}

//-------------------------------------------------------------------------
vtkPieceList::~vtkPieceList()
{
  this->Clear();
  delete this->Internals;
}

//-------------------------------------------------------------------------
void vtkPieceList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
void vtkPieceList::AddPiece(vtkPiece *piece)
{
  this->Internals->Pieces.push_back(piece);
  piece->Register(this);
}

//-------------------------------------------------------------------------
vtkPiece *vtkPieceList::GetPiece(int n)
{
  if (this->Internals->Pieces.size() > (unsigned int)n)
    {
    return this->Internals->Pieces[n];
    }
  return NULL;
}

//-------------------------------------------------------------------------
void vtkPieceList::RemovePiece(int n)
{
  if (this->Internals->Pieces.size() > (unsigned int)n)
    {
    this->Internals->Pieces[n]->UnRegister(this);
    this->Internals->Pieces.erase(this->Internals->Pieces.begin()+n);
    }
}

//-------------------------------------------------------------------------
void vtkPieceList::Clear()
{
  for (unsigned int i = 0; i < this->Internals->Pieces.size(); i++)
    {
    this->Internals->Pieces[i]->UnRegister(this);
    }  
  this->Internals->Pieces.clear();
}

//-------------------------------------------------------------------------
int vtkPieceList::GetNumberOfPieces()
{
  return this->Internals->Pieces.size();
}

//-------------------------------------------------------------------------
int vtkPieceList::GetNumberNonZeroPriority()
{
  int last = this->Internals->Pieces.size()-1;
  for (int i = last; i >= 0 ; --i)
    {
    if (this->Internals->Pieces[i]->GetPriority() > 0.0)
      {
      return i+1;
      }
    }
  return 0;
}

//-------------------------------------------------------------------------
void vtkPieceList::SortPriorities()
{
  vtkstd::sort(this->Internals->Pieces.begin(),
       this->Internals->Pieces.end(),
       vtkPieceListByPriority());
}

//-------------------------------------------------------------------------
void vtkPieceList::Print()
{
  int np = this->GetNumberOfPieces();
  cerr << "PL(" << this << "):" << np << " [";
  for (int i = 0; i < np; i++)
    {
    cerr << "{"
         << this->GetPiece(i)->GetPiece() << "/"
         << this->GetPiece(i)->GetNumPieces() << "@"
         << this->GetPiece(i)->GetResolution() << "="
         << this->GetPiece(i)->GetPriority()
         << "},";
    }
  cerr << "]" << endl;
}

//-------------------------------------------------------------------------
void vtkPieceList::Serialize()
{  
  if (this->Internals->SerializeBuffer != NULL)
    {
    delete[] this->Internals->SerializeBuffer;
    this->Internals->BufferSize = 0;
    }
  int np = this->GetNumberOfPieces();
  this->Internals->SerializeBuffer = new double[1+np*sizeof(vtkPiece)];
  double *ptr = this->Internals->SerializeBuffer;
  *ptr = (double)np;
  ptr++;
  double *optr = NULL;
  for (int i = 0; i < np; i++)
    {
    vtkPiece *mine = this->GetPiece(i);
    mine->Serialize(ptr, &optr);
    ptr = optr;
    }
  this->Internals->BufferSize = optr - this->Internals->SerializeBuffer;
}

//-------------------------------------------------------------------------
void vtkPieceList::GetSerializedList(double **ret, int *size)
{
  if (!ret || !size)
    {
    return;
    }
  *ret = this->Internals->SerializeBuffer;
  *size = this->Internals->BufferSize;
}

//-------------------------------------------------------------------------
void vtkPieceList::UnSerialize(double *buff)
{
  this->Clear();
  if (!buff)
    {
    return;
    }
  double *ptr = buff;
  int np = (int)*ptr;
  ptr++;
  for (int i = 0; i < np; i++)
    {
    double *optr = NULL;
    vtkPiece *mine = vtkPiece::New();
    mine->UnSerialize(ptr, &optr);
    ptr = optr;
    this->AddPiece(mine);
    mine->Delete();
    }
}

//-------------------------------------------------------------------------
vtkPiece *vtkPieceList::PopPiece(int n /*= 0*/)
{
  vtkPiece *p = this->GetPiece(n);
  p->Register(NULL);
  this->RemovePiece(n);
  return p;
}

//-------------------------------------------------------------------------
void vtkPieceList::CopyPieceList(vtkPieceList *other)
{
  this->CopyInternal(other, 0);
}
//-------------------------------------------------------------------------
void vtkPieceList::MergePieceList(vtkPieceList *other)
{
  this->CopyInternal(other, 1);
}

//-------------------------------------------------------------------------
void vtkPieceList::CopyInternal(vtkPieceList *other, int merge)
{
  if (!merge)
    {
    this->Clear();
    }
  if (!other)
    {
    return;
    }
  for (int i = 0; i < other->GetNumberOfPieces(); i++)
    {
    vtkPiece *mine = vtkPiece::New();
    vtkPiece *his = other->GetPiece(i);
    mine->CopyPiece(his);
    this->AddPiece(mine);
    mine->Delete();
    }
  if (merge)
    {
    other->Clear();
    }
}

