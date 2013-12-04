/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __CartesianBounds_h
#define __CartesianBounds_h

#include <algorithm> // for min and max
#include <iostream> // for ostream
#include <cmath> // for pow and log10

/// Representation of a cartesian volume.
/**
Represnetation of a cartesian volume and common operations
on it. The implementation is intended to be fast and light
so that it may be used in place of double[6] with no
performance penalty.
*/
class CartesianBounds
{
public:
  CartesianBounds();
  CartesianBounds(const double bounds[6]);
  CartesianBounds(
      double xlo,
      double xhi,
      double ylo,
      double yhi,
      double zlo,
      double zhi);
  CartesianBounds(const CartesianBounds &other);
  CartesianBounds &operator=(const CartesianBounds &other);

  /// \Section Accessors @\{
  /**
  Set the bounds.
  */
  void Set(const double bounds[6]);
  void Set(double xlo, double xhi, double ylo, double yhi, double zlo, double zhi);
  void Set(const CartesianBounds &bounds);
  void Clear();

  /**
  Direct access to internal data.
  */
  double *GetData(){ return this->Data; }
  const double *GetData() const { return this->Data; }

  /**
  Array access
  */
  double &operator[](int i){ return this->Data[i]; }
  const double &operator[](int i) const { return this->Data[i]; }
  /// \@}


  /// \Section Tests @\{
  /**
  Return non-zero if empty.
  */
  int Empty() const;

  /**
  Inside evaluates true if the point is strictly inside or on the bounds.
  */
  int Inside(const double pt[3]) const;

  /**
  Outside evaluates true if the point is strictly outside the bounds.
  */
  int Outside(const double pt[3]) const;

  /**
  Evaluates true if the given bounds is inside or coincident with this
  bounds.
  */
  int Inside(const CartesianBounds &other) const;

  /**
  Evaluates true if the given bounds is strictly outside this bounds.
  */
  int Outside(const CartesianBounds &other) const;

  /// \@}


  /// \Section Operators @\{

  /// NOTE operations on empty bounds produce incorrect results
  /// in most cases. If it's an issue explicitly test with Empty().

  /**
  In place intersection.
  */
  void operator&=(const CartesianBounds &other);

  /**
  Expand the bounds by g.
  */
  void Grow(double g);
  void Grow(int q, double g);

  /**
  Shrink the bounds by s.
  */
  void Shrink(double s);
  void Shrink(int q, double s);

  /**
  Shrink the bounds by the least significant digit
  in each of 6 directions. This is smallest amount
  numerically realizable by 64 bit IEEE float.
  */
  void ShrinkScaledEpsilon();

  /**
  Translate the bounds.
  */
  void Shift(double s);
  void Shift(int q, double s);
  /// @\}

private:
  double Data[6];
};

std::ostream &operator<<(std::ostream &os,const CartesianBounds &bounds);

class vtkUnstructuredGrid;
vtkUnstructuredGrid &operator<<(vtkUnstructuredGrid &, const CartesianBounds &bounds);

//-----------------------------------------------------------------------------
inline
CartesianBounds::CartesianBounds()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
inline
CartesianBounds::CartesianBounds(const double bounds[6])
{
  this->Set(bounds);
}

//-----------------------------------------------------------------------------
inline
CartesianBounds::CartesianBounds(
      double xlo,
      double xhi,
      double ylo,
      double yhi,
      double zlo,
      double zhi)
{
  this->Set(xlo,xhi,ylo,yhi,zlo,zhi);
}

//-----------------------------------------------------------------------------
inline
CartesianBounds::CartesianBounds(const CartesianBounds &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
inline
CartesianBounds &CartesianBounds::operator=(const CartesianBounds &other)
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
void CartesianBounds::Set(const CartesianBounds &other)
{
  this->Set(other.GetData());
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Set(const double bounds[6])
{
  Data[0]=bounds[0];
  Data[1]=bounds[1];
  Data[2]=bounds[2];
  Data[3]=bounds[3];
  Data[4]=bounds[4];
  Data[5]=bounds[5];
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Set(
      double xlo,
      double xhi,
      double ylo,
      double yhi,
      double zlo,
      double zhi)
{
  double I[6]={xlo,xhi,ylo,yhi,zlo,zhi};
  this->Set(I);
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Clear()
{
  this->Set(1,0,1,0,1,0);
}

//-----------------------------------------------------------------------------
inline
int CartesianBounds::Empty() const
{
  if ( this->Data[0]>=this->Data[1]
    || this->Data[2]>=this->Data[3]
    || this->Data[4]>=this->Data[5])
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int CartesianBounds::Inside(const double pt[3]) const
{
  if ( pt[0]>=this->Data[0]
    && pt[0]<=this->Data[1]
    && pt[1]>=this->Data[2]
    && pt[1]<=this->Data[3]
    && pt[2]>=this->Data[4]
    && pt[2]<=this->Data[5] )
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
inline
int CartesianBounds::Outside(const double pt[3]) const
{
  if ( pt[0]>=this->Data[0]
    && pt[0]<=this->Data[1]
    && pt[1]>=this->Data[2]
    && pt[1]<=this->Data[3]
    && pt[2]>=this->Data[4]
    && pt[2]<=this->Data[5] )
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
inline
int CartesianBounds::Inside(const CartesianBounds &other) const
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
int CartesianBounds::Outside(const CartesianBounds &other) const
{
  if ( (this->Data[0]<=other.Data[0])
    && (this->Data[1]>=other.Data[1])
    && (this->Data[2]<=other.Data[2])
    && (this->Data[3]>=other.Data[3])
    && (this->Data[4]<=other.Data[4])
    && (this->Data[5]>=other.Data[5]) )
    {
    return 0;
    }
  return 1;
}


//-----------------------------------------------------------------------------
inline
void CartesianBounds::operator&=(const CartesianBounds &other)
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
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Grow(double g)
{
  this->Data[0]-=g;
  this->Data[1]+=g;
  this->Data[2]-=g;
  this->Data[3]+=g;
  this->Data[4]-=g;
  this->Data[5]+=g;
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Grow(int q, double g)
{
  q*=2;

  this->Data[q  ]-=g;
  this->Data[q+1]+=g;
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::ShrinkScaledEpsilon()
{
  for (int i=0; i<6; ++i)
    {
    double eps=((i%2)==0?1.0E-15:-1.0E-15);
    if (this->Data[i]>1.0)
        {
        int s=(int)std::log10(this->Data[i]);
        eps*=std::pow(10.0,s);
        }
    this->Data[i]=this->Data[i]+eps;
    }
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Shrink(double s)
{
  this->Data[0]+=s;
  this->Data[1]-=s;
  this->Data[2]+=s;
  this->Data[3]-=s;
  this->Data[4]+=s;
  this->Data[5]-=s;
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Shrink(int q, double s)
{
  q*=2;

  this->Data[q  ]+=s;
  this->Data[q+1]-=s;
}


//-----------------------------------------------------------------------------
inline
void CartesianBounds::Shift(double s)
{
  this->Data[0]+=s;
  this->Data[1]+=s;
  this->Data[2]+=s;
  this->Data[3]+=s;
  this->Data[4]+=s;
  this->Data[5]+=s;
}

//-----------------------------------------------------------------------------
inline
void CartesianBounds::Shift(int q, double s)
{
  q*=2;

  this->Data[q  ]+=s;
  this->Data[q+1]+=s;
}

#endif

// VTK-HeaderTest-Exclude: CartesianBounds.h
