#include "vtkPieceList.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/algorithm>
#include <vtksys/stl/vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPieceList);

//////////////////////////////////////////////////////////////////////////////
class vtkPieceList::Internal
{
public:
  Internal()
  {
    this->SerializeBuffer = NULL;
    this->BufferSize = 0;
  }
  ~Internal()
  {
  if (this->SerializeBuffer != NULL)
    {
    delete[] this->SerializeBuffer;
    }
  }
  vtksys_stl::vector<vtkPiece> Pieces;
  char *SerializeBuffer;
  int BufferSize;
};

class vtkPieceListByPriority
{
public:
  bool operator() (vtkPiece one, vtkPiece two)
  {
  return one.ComparePriority(two);
  }
};

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
vtkPieceList::vtkPieceList()
{
  //cerr << "PL(" <<this<< ") create" << endl;
  this->Internals = new Internal;
}

//----------------------------------------------------------------------------
vtkPieceList::~vtkPieceList()
{
  //cerr << "PL(" <<this<< ") delete" << endl;
  this->Clear();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkPieceList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPieceList::AddPiece(vtkPiece piece)
{
  this->Internals->Pieces.push_back(piece);
}

//----------------------------------------------------------------------------
vtkPiece vtkPieceList::GetPiece(int n)
{
  if (this->Internals->Pieces.size() > (unsigned int)n)
    {
    return this->Internals->Pieces[n];
    }
  vtkPiece p;
  p.SetPiece(-1);
  return p;
}

//----------------------------------------------------------------------------
void vtkPieceList::SetPiece(int n, vtkPiece other)
{
  if (this->Internals->Pieces.size() > (unsigned int)n)
    {
    this->Internals->Pieces[n] = other;
    }
}

//----------------------------------------------------------------------------
void vtkPieceList::RemovePiece(int n)
{
  if (this->Internals->Pieces.size() > (unsigned int)n)
    {
    this->Internals->Pieces.erase(this->Internals->Pieces.begin()+n);
    }
}

//------------------------------------------------------------------------------
vtkPiece vtkPieceList::PopPiece(int n /*= 0*/)
{
  vtkPiece p = this->GetPiece(n);
  this->RemovePiece(n);
  return p;
}

//----------------------------------------------------------------------------
void vtkPieceList::Clear()
{
  this->Internals->Pieces.clear();
}

//-----------------------------------------------------------------------------
int vtkPieceList::GetNumberOfPieces()
{
  return this->Internals->Pieces.size();
}

//------------------------------------------------------------------------------
int vtkPieceList::GetNumberNonZeroPriority()
{
  int last = this->Internals->Pieces.size()-1;
  for (int i = last; i >= 0 ; --i)
    {
    if (this->Internals->Pieces[i].GetPriority() > 0.0)
      {
      return i+1;
      }
    }
  return 0;
}

//------------------------------------------------------------------------------
void vtkPieceList::SortPriorities()
{
  vtksys_stl::sort(this->Internals->Pieces.begin(),
                   this->Internals->Pieces.end(),
                   vtkPieceListByPriority());
}

//-----------------------------------------------------------------------------
void vtkPieceList::Print()
{
  int np = this->GetNumberOfPieces();
  cerr << "PL(" << this << "):" << np << " \n[";
  for (int i = 0; i < np; i++)
    {
    cerr
      << "{"
      << this->GetPiece(i).GetProcessor() << ":"
      << this->GetPiece(i).GetPiece() << "/"
      << this->GetPiece(i).GetNumPieces() << "@"
      << this->GetPiece(i).GetResolution() << "->["
      << this->GetPiece(i).GetBounds()[0] << "-"
      << this->GetPiece(i).GetBounds()[1] << ","
      << this->GetPiece(i).GetBounds()[2] << "-"
      << this->GetPiece(i).GetBounds()[3] << ","
      << this->GetPiece(i).GetBounds()[4] << "-"
      << this->GetPiece(i).GetBounds()[5] << "]=("
      << this->GetPiece(i).GetPipelinePriority() << " "
      << this->GetPiece(i).GetViewPriority() << " "
      << this->GetPiece(i).GetCachedPriority() << ")"
      << "},\n";
    }
  cerr << "]" << endl;
}

//----------------------------------------------------------------------------
void vtkPieceList::Serialize()
{
  if (this->Internals->SerializeBuffer != NULL)
    {
    delete[] this->Internals->SerializeBuffer;
    this->Internals->BufferSize = 0;
    }
  vtksys_ios::ostringstream temp;
  int np = this->GetNumberOfPieces();
  temp << np << " ";
  for (int i = 0; i < np; i++)
    {
    vtkPiece mine = this->GetPiece(i);
    temp << mine.Processor << " "
         << mine.Piece << " "
         << mine.NumPieces << " "
         << mine.Resolution << " "
         << mine.Bounds[0] << " "
         << mine.Bounds[1] << " "
         << mine.Bounds[2] << " "
         << mine.Bounds[3] << " "
         << mine.Bounds[4] << " "
         << mine.Bounds[5] << " "
         << mine.PipelinePriority << " "
         << mine.ViewPriority << " "
         << mine.CachedPriority << " ";
    }
  int len = strlen(temp.str().c_str());
  this->Internals->SerializeBuffer = new char[len+10];
  strcpy(this->Internals->SerializeBuffer, temp.str().c_str());
  this->Internals->BufferSize = len;
}

//----------------------------------------------------------------------------
void vtkPieceList::GetSerializedList(char **ret, int *size)
{
  if (!ret || !size)
    {
    return;
    }
  *ret = this->Internals->SerializeBuffer;
  *size = this->Internals->BufferSize;
}

//----------------------------------------------------------------------------
void vtkPieceList::UnSerialize(char *buffer, int *bytes)
{
  this->Clear();
  if (!buffer || !bytes)
    {
    return;
    }
  vtksys_ios::istringstream temp;
  temp.str(buffer);

  int start = temp.tellg();

  int np;
  temp >> np;
  for (int i = 0; i < np; i++)
    {
    vtkPiece mine;;
    temp >> mine.Processor;
    temp >> mine.Piece;
    temp >> mine.NumPieces;
    temp >> mine.Resolution;
    temp >> mine.Bounds[0];
    temp >> mine.Bounds[1];
    temp >> mine.Bounds[2];
    temp >> mine.Bounds[3];
    temp >> mine.Bounds[4];
    temp >> mine.Bounds[5];
    temp >> mine.PipelinePriority;
    temp >> mine.ViewPriority;
    temp >> mine.CachedPriority;
    this->AddPiece(mine);
    }
  int end = temp.tellg();
  *bytes = end-start;
}


//------------------------------------------------------------------------------
void vtkPieceList::CopyPieceList(vtkPieceList *other)
{
  this->CopyInternal(other, 0);
}

//------------------------------------------------------------------------------
void vtkPieceList::MergePieceList(vtkPieceList *other)
{
  this->CopyInternal(other, 1);
}

//------------------------------------------------------------------------------
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
    vtkPiece mine;
    vtkPiece his = other->GetPiece(i);
    mine.CopyPiece(his);
    this->AddPiece(mine);
    }
  if (merge)
    {
    other->Clear();
    }
}

//------------------------------------------------------------------------------
void vtkPieceList::DummyFill()
{
  //Used for testing, it makes up a small piecelist with known content.
  static int myid = 0;
  //autoincrements to make it easy to identify which piecelist each piece
  //was originally generated by after it has been shuffled around
  this->Clear();
  int numPieces=5;
  for (int i = 0; i < numPieces; i++)
    {
    vtkPiece mine;
    mine.SetPiece(i);
    mine.SetNumPieces(numPieces);
    mine.SetResolution(myid);
    mine.SetPipelinePriority((double)i/(double)numPieces);
    this->AddPiece(mine);
    }
  myid++;
}

//------------------------------------------------------------------------------
void vtkPieceList::PrintSerializedList()
{
  char *buffer;
  int len;
  this->GetSerializedList(&buffer, &len);
  cerr << "LEN = " << len << endl;
  cerr << buffer << endl;
}

//------------------------------------------------------------------------------
void vtkPieceList::CopyBuddy(vtkPieceList *buddy)
{
  if (!buddy)
    {
    cerr << "WHO?" << endl;
    return;
    }
  buddy->Serialize();

  char *buffer;
  int len;
  buddy->GetSerializedList(&buffer, &len);
  //cerr << "LEN = " << len << endl;
  //cerr << buffer << endl;

  this->UnSerialize(buffer, &len);
  //this->Print();
}
