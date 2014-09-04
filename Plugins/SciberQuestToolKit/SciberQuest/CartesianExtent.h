/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CartesianExtent_h
#define __CartesianExtent_h

#include <algorithm> // for min and max
#include <iostream> // for ostream

/// Index space representation of a cartesian volume.
/**
Represnetation of a cartesian volume and common operations
on it. The implementation is intended to be fast and light
so that it may be used in place of int[6] with little or no
performance penalty.
*/
class CartesianExtent
{
public:
  CartesianExtent();
  CartesianExtent(const int *ext);
  CartesianExtent(int ilo, int ihi, int jlo, int jhi, int klo, int khi);
  CartesianExtent(const CartesianExtent &other);

  CartesianExtent &operator=(const CartesianExtent &other);

  /// \Section Accessors \@{
  /**
  Element access
  */
  int &operator[](int i){ return this->Data[i]; }
  const int &operator[](int i) const { return this->Data[i]; }

  /**
  Set the extent.
  */
  void Set(const CartesianExtent &ext);
  void Set(const int ext[6]);
  void Set(int ilo, int ihi, int jlo, int jhi, int klo, int khi);
  void Clear();

  /**
  Direct access to internal data.
  */
  void GetData(int data[6]) const;
  int *GetData(){ return this->Data; }
  const int *GetData() const { return this->Data; }

  /**
  Get the start/end index relative to an origin.
  */
  void GetStartIndex(int first[3]) const;
  void GetStartIndex(int first[3], const int origin[3]) const;
  void GetEndIndex(int last[3]) const;

  /**
  Given the dataset origin (point that coresponds to the index 0,0,0
  compute the point at the start of this extent.
  */
  void GetLowerBound(
        const double X0[3],
        const double DX[3],
        double lowerBound[3]) const;
  void GetLowerBound(
        const float *X,
        const float *Y,
        const float *Z,
        double lowerBound[3]) const;

  /**
  Given the dataset origin (point that coresponds to the index 0,0,0)
  and the dataset spacing compute the point at the end of this extent.
  */
  void GetUpperBound(
        const double X0[3],
        const double DX[3],
        double upperBound[3]) const;
  void GetUpperBound(
        const float *X,
        const float *Y,
        const float *Z,
        double upperBound[3]) const;

  /**
  Given the dataset origin (point that coresponds to the index 0,0,0)
  and the dataset spacing compute the point at the end of this extent.
  WARNING: this produces incorrect result for 2D data
  void GetBounds(
        const double X0[3],
        const double DX[3],
        double bounds[6]) const;

  void GetBounds(
        const float *X,
        const float *Y,
        const float *Z,
        double bounds[6]) const;
  */
  /// \@}


  /// \Section Queries \@{
  /**
  Return true if empty.
  */
  int Empty() const;

  /**
  Test for equivalence.
  */
  int operator==(const CartesianExtent &other) const;

  /**
  Return non-zero if this extent conatins the other.
  */
  int Contains(const CartesianExtent &other) const;

  /**
  Get the number in each direction.
  */
  template<typename T>
  void Size(T nCells[3]) const;

  /**
  Get the total number.
  */
  size_t Size() const;
  /// \@}


  /// \Section Operators \@{
  /// NOTE in most cases operation on an empty object produces
  /// incorrect results. If it an issue query Empty().
  /**
  In place intersection.
  */
  void operator&=(const CartesianExtent &other);

  /**
  Expand the extents by n.
  */
  void Grow(int n);
  void Grow(int q, int n);
  void GrowLow(int q, int n);
  void GrowHigh(int q, int n);

  /**
  Shrink the extent by n.
  */
  void Shrink(int n);
  void Shrink(int q, int n);

  /**
  Translate the bounds by n.
  */
  void Shift();                           // shift by low corner of this
  void Shift(const CartesianExtent &ext); // shift by low corner of the given extent
  void Shift(int ni, int nj, int nk);     // shift by the given amount
  void Shift(int q, int n);               // shift by the given amount in the given direction

  /**
  Divide the extent in half in the given direction. The
  operation is done in-place the other half of the split
  extent is returned. The retunr will be empty if the split
  could not be made.
  */
  CartesianExtent Split(int dir);

  /**
  In-place conversion from cell based to node based extent, and vise-versa.
  */
  void CellToNode();
  void NodeToCell();

  /// \@}


  /// \Section Ghost Cell Manipulations \@{
  enum {
    DIM_MODE_INVALID=-1,
    DIM_MODE_3D,
    DIM_MODE_2D_XY,
    DIM_MODE_2D_XZ,
    DIM_MODE_2D_YZ
    };

  /**
  Determine if we have 3D or 2D data and which plane we
  are in.
  */
  static int GetDimensionMode(
      const CartesianExtent &problemDomain,
      int nGhosts);

  static int GetDimensionMode(
      const CartesianExtent &problemDomain);

  /**
  Get the number in each direction.
  */
  template<typename T>
  static
  void Size(const CartesianExtent &ext, T nCells[3]);

  /**
  Get the total number.
  */
  static
  size_t Size(const CartesianExtent &ext);

  /**
  Add or remove ghost cells. If a problem domain is
  provided then the result is clipled to be within the
  problem domain.
  */
  static CartesianExtent Grow(
      const CartesianExtent &inputExt,
      int nGhosts,
      int mode);

  static CartesianExtent Grow(
      const CartesianExtent &inputExt,
      const CartesianExtent &problemDomain,
      int nGhosts,
      int mode);

  static CartesianExtent GrowLow(
      const CartesianExtent &ext,
      int q,
      int n,
      int mode);

  static CartesianExtent GrowHigh(
      const CartesianExtent &ext,
      int q,
      int n,
      int mode);

  /**
  Remove ghost cells. If a problem domain is
  provided the input is pinned at the domain.
  */
  static CartesianExtent Shrink(
      const CartesianExtent &inputExt,
      const CartesianExtent &problemDomain,
      int nGhosts,
      int mode);

  static CartesianExtent Shrink(
      const CartesianExtent &inputExt,
      int nGhosts,
      int mode);

   /**
   Convert from point extent to cell extent
   while respecting the dimensionality of the data.
   */
   static CartesianExtent NodeToCell(
      const CartesianExtent &inputExt,
      int mode);

   /**
   Convert from cell extent to point extent
   while respecting the dimensionality of the data.
   */
   static CartesianExtent CellToNode(
      const CartesianExtent &inputExt,
      int mode);
  /**
  shift by the given amount while respecting mode
  */
  static void Shift(int *ijk, int n, int mode);
  static void Shift(int *ijk, int *n, int mode);

  /**
  Given the dataset origin (point that coresponds to the index 0,0,0
  compute the point at the start of this extent.
  */
  static void GetLowerBound(
        const CartesianExtent &ext,
        const double X0[3],
        const double DX[3],
        double lowerBound[3]);

  static void GetLowerBound(
        const CartesianExtent &ext,
        const float *X,
        const float *Y,
        const float *Z,
        double lowerBound[3]);

  /**
  Given the dataset origin (point that coresponds to the index 0,0,0)
  and the dataset spacing compute the point at the end of this extent.
  respecting mode
  */
  static void GetBounds(
        const CartesianExtent &ext,
        const double X0[3],
        const double DX[3],
        int mode,
        double bounds[6]);

  static void GetBounds(
        const CartesianExtent &ext,
        const float *X,
        const float *Y,
        const float *Z,
        int mode,
        double bounds[6]);


  /// \@}

private:
  int Data[6];
};

std::ostream &operator<<(std::ostream &os, const CartesianExtent &ext);

//-----------------------------------------------------------------------------
inline
CartesianExtent::CartesianExtent()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
inline
CartesianExtent::CartesianExtent(const int ext[6])
{
  this->Set(ext);
}

//-----------------------------------------------------------------------------
inline
CartesianExtent::CartesianExtent(
      int ilo,
      int ihi,
      int jlo,
      int jhi,
      int klo,
      int khi)
{
  this->Set(ilo,ihi,jlo,jhi,klo,khi);
}

//-----------------------------------------------------------------------------
inline
CartesianExtent::CartesianExtent(const CartesianExtent &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Set(const CartesianExtent &other)
{
  this->Set(other.GetData());
}

//-----------------------------------------------------------------------------
inline
CartesianExtent &CartesianExtent::operator=(const CartesianExtent &other)
{
  if (&other==this)
    {
    return *this;
    }
  this->Set(other);

  return *this;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Set(const int ext[6])
{
  Data[0]=ext[0];
  Data[1]=ext[1];
  Data[2]=ext[2];
  Data[3]=ext[3];
  Data[4]=ext[4];
  Data[5]=ext[5];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Set(int ilo, int ihi, int jlo, int jhi, int klo, int khi)
{
  int I[6]={ilo,ihi,jlo,jhi,klo,khi};
  this->Set(I);
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Clear()
{
  this->Set(1,0,1,0,1,0);
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetData(int data[6]) const
{
  data[0]=this->Data[0];
  data[1]=this->Data[1];
  data[2]=this->Data[2];
  data[3]=this->Data[3];
  data[4]=this->Data[4];
  data[5]=this->Data[5];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetLowerBound(
      const double X0[3],
      const double DX[3],
      double lowerBound[3]) const
{
  lowerBound[0]=X0[0]+this->Data[0]*DX[0];
  lowerBound[1]=X0[1]+this->Data[2]*DX[1];
  lowerBound[2]=X0[2]+this->Data[4]*DX[2];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetLowerBound(
      const float *X,
      const float *Y,
      const float *Z,
      double lowerBound[3]) const
{
  lowerBound[0]=X[this->Data[0]];
  lowerBound[1]=Y[this->Data[2]];
  lowerBound[2]=Z[this->Data[4]];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetUpperBound(
      const double X0[3],
      const double DX[3],
      double upperBound[3]) const
{
  int nCells[3];
  this->Size(nCells);

  double extX0[3];
  this->GetLowerBound(X0,DX,extX0);

  upperBound[0]=extX0[0]+nCells[0]*DX[0];
  upperBound[1]=extX0[1]+nCells[1]*DX[1];
  upperBound[2]=extX0[2]+nCells[2]*DX[2];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetUpperBound(
      const float *X,
      const float *Y,
      const float *Z,
      double upperBound[3]) const
{
  upperBound[0]=X[this->Data[1]+1];
  upperBound[1]=Y[this->Data[3]+1];
  upperBound[2]=Z[this->Data[5]+1];
}

/*
//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetBounds(
      const double X0[3],
      const double DX[3],
      double bounds[6]) const
{
  int nCells[3];
  this->Size(nCells);

  double extX0[3];
  this->GetLowerBound(X0,DX,extX0);

  bounds[0]=extX0[0];
  bounds[1]=extX0[0]+nCells[0]*DX[0];
  bounds[2]=extX0[1];
  bounds[3]=extX0[1]+nCells[1]*DX[1];
  bounds[4]=extX0[2];
  bounds[5]=extX0[2]+nCells[2]*DX[2];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetBounds(
      const float *X,
      const float *Y,
      const float *Z,
      double bounds[6]) const
{

  bounds[0]=X[this->Data[0]];
  bounds[1]=X[this->Data[1]+1];
  bounds[2]=Y[this->Data[2]];
  bounds[3]=Y[this->Data[3]+1];
  bounds[4]=Z[this->Data[4]];
  bounds[5]=Z[this->Data[5]+1];
}
*/

//-----------------------------------------------------------------------------
template<typename T>
void CartesianExtent::Size(T nCells[3]) const
{
  CartesianExtent::Size(*this,nCells);
}

//-----------------------------------------------------------------------------
inline
size_t CartesianExtent::Size() const
{
  return CartesianExtent::Size(*this);
}

//-----------------------------------------------------------------------------
template<typename T>
void CartesianExtent::Size(const CartesianExtent &ext, T nCells[3])
{
  nCells[0]=static_cast<T>(ext[1])-static_cast<T>(ext[0])+1;
  nCells[1]=static_cast<T>(ext[3])-static_cast<T>(ext[2])+1;
  nCells[2]=static_cast<T>(ext[5])-static_cast<T>(ext[4])+1;
}

//-----------------------------------------------------------------------------
inline
size_t CartesianExtent::Size(const CartesianExtent &ext)
{
  return
       (static_cast<size_t>(ext[1])-static_cast<size_t>(ext[0])+1)
      *(static_cast<size_t>(ext[3])-static_cast<size_t>(ext[2])+1)
      *(static_cast<size_t>(ext[5])-static_cast<size_t>(ext[4])+1);
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetStartIndex(int first[3]) const
{
  first[0]=this->Data[0];
  first[1]=this->Data[2];
  first[2]=this->Data[4];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetStartIndex(int first[3], const int origin[3]) const
{
  first[0]=this->Data[0]-origin[0];
  first[1]=this->Data[2]-origin[1];
  first[2]=this->Data[4]-origin[2];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GetEndIndex(int last[3]) const
{
  last[0]=this->Data[1];
  last[1]=this->Data[3];
  last[2]=this->Data[5];
}

//-----------------------------------------------------------------------------
inline
int CartesianExtent::Empty() const
{
  if ( this->Data[0]>this->Data[1]
    || this->Data[2]>this->Data[3]
    || this->Data[4]>this->Data[5])
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int CartesianExtent::operator==(const CartesianExtent &other) const
{
  if ( (this->Data[0]==other.Data[0])
    && (this->Data[1]==other.Data[1])
    && (this->Data[2]==other.Data[2])
    && (this->Data[3]==other.Data[3])
    && (this->Data[4]==other.Data[4])
    && (this->Data[5]==other.Data[5]) )
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int CartesianExtent::Contains(const CartesianExtent &other) const
{
  if ( (this->Data[0]<=other.Data[0])
    && (this->Data[1]>=other.Data[1])
    && (this->Data[2]<=other.Data[2])
    && (this->Data[3]>=other.Data[3])
    && (this->Data[4]<=other.Data[4])
    && (this->Data[5]>=other.Data[5]) )
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::operator&=(const CartesianExtent &other)
{
  if (this->Empty())
    {
    return;
    }

  if (other.Empty())
    {
    this->Clear();
    return;
    }

  this->Data[0]=std::max(this->Data[0],other.Data[0]);
  this->Data[1]=std::min(this->Data[1],other.Data[1]);
  this->Data[2]=std::max(this->Data[2],other.Data[2]);
  this->Data[3]=std::min(this->Data[3],other.Data[3]);
  this->Data[4]=std::max(this->Data[4],other.Data[4]);
  this->Data[5]=std::min(this->Data[5],other.Data[5]);

  if (this->Empty())
    {
    this->Clear();
    }
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Grow(int n)
{
  this->Data[0]-=n;
  this->Data[1]+=n;
  this->Data[2]-=n;
  this->Data[3]+=n;
  this->Data[4]-=n;
  this->Data[5]+=n;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Grow(int q, int n)
{
  q*=2;

  this->Data[q  ]-=n;
  this->Data[q+1]+=n;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GrowLow(int q, int n)
{
  this->Data[2*q]-=n;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::GrowHigh(int q, int n)
{
  this->Data[2*q+1]+=n;
}



//-----------------------------------------------------------------------------
inline
void CartesianExtent::Shrink(int n)
{
  this->Data[0]+=n;
  this->Data[1]-=n;
  this->Data[2]+=n;
  this->Data[3]-=n;
  this->Data[4]+=n;
  this->Data[5]-=n;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Shrink(int q, int n)
{
  q*=2;

  this->Data[q  ]+=n;
  this->Data[q+1]-=n;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Shift(int ni, int nj, int nk)
{
  this->Data[0]+=ni;
  this->Data[1]+=ni;
  this->Data[2]+=nj;
  this->Data[3]+=nj;
  this->Data[4]+=nk;
  this->Data[5]+=nk;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Shift(int q, int n)
{
  q*=2;

  this->Data[q  ]+=n;
  this->Data[q+1]+=n;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Shift(const CartesianExtent &other)
{
  for (int q=0; q<3; ++q)
    {
    int qq=q*2;
    int n=-other[qq];

    this->Data[qq  ]+=n;
    this->Data[qq+1]+=n;
    }
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::Shift()
{
  for (int q=0; q<3; ++q)
    {
    int qq=q*2;
    int n=-this->Data[qq];

    this->Data[qq  ]+=n;
    this->Data[qq+1]+=n;
    }
}

//-----------------------------------------------------------------------------
inline
CartesianExtent CartesianExtent::Split(int dir)
{
  CartesianExtent half;

  int q=2*dir;
  int l=this->Data[q+1]-this->Data[q]+1;
  int s=l/2;

  if (s)
    {
    s+=this->Data[q];
    half=*this;
    half.Data[q]=s;
    this->Data[q+1]=s-1;
    }

  return half;
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::CellToNode()
{
  ++this->Data[1];
  ++this->Data[3];
  ++this->Data[5];
}

//-----------------------------------------------------------------------------
inline
void CartesianExtent::NodeToCell()
{
  --this->Data[1];
  --this->Data[3];
  --this->Data[5];
}

#endif

// VTK-HeaderTest-Exclude: CartesianExtent.h
