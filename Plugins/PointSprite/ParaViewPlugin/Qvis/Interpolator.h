/*****************************************************************************
*
* Copyright (c) 2000 - 2007, The Regents of the University of California
* Produced at the Lawrence Livermore National Laboratory
* All rights reserved.
*
* This file is part of VisIt. For details, see http://www.llnl.gov/visit/. The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or materials provided with the distribution.
*  - Neither the name of the UC/LLNL nor  the names of its contributors may be
*    used to  endorse or  promote products derived from  this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED.  IN  NO  EVENT  SHALL  THE  REGENTS  OF  THE  UNIVERSITY OF
* CALIFORNIA, THE U.S.  DEPARTMENT  OF  ENERGY OR CONTRIBUTORS BE  LIABLE  FOR
* ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include <vtkstd/vector>

// ****************************************************************************
//  Class:  ConstInterp
//
//  Purpose:
//    Interpolators for piecewise constant interpolation.
//    Rounds to nearest control point.
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 17, 2003
//
// ****************************************************************************
template <class T>
struct ConstInterp
{
    inline static void InterpScalar(void *, void *a1,void *a2,        double);
    inline static void InterpArray(void *,  void *a1,void *a2, int l, double);
    inline static void InterpVector(void *, void *a1,void *a2,        double);
};

// ****************************************************************************
//  Class:  LinInterp
//
//  Purpose:
//    Interpolators for piecewise linear interpolation.
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 17, 2003
//
// ****************************************************************************
template <class T>
struct LinInterp
{
    inline static void InterpScalar(void *, void *a1,void *a2,        double);
    inline static void InterpArray(void *,  void *a1,void *a2, int l, double);
    inline static void InterpVector(void *, void *a1,void *a2,        double);
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                               Inline Methods
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------



// ****************************************************************************
//  Methods:  ConstInterp<T>::Interp*
//
//  Purpose:
//    Methods for piecewise constant interpolation.
//    Rounds to nearest control point.
//
//  Arguments:
//    out        the pointer to the output location
//    a1         the pointer to the first  control point value
//    a2         the pointer to the second control point value
//    l          (where applicable) the length of the array
//    f          the alpha value from [0.0 , 1.0]
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 17, 2003
//
// ****************************************************************************
template <class T>
void
ConstInterp<T>::InterpScalar(void *out_, void *a1_, void *a2_, double f)
{
    T *out = (T*)out_;
    T *a1  = (T*)a1_;
    T *a2  = (T*)a2_;
    if (f < .5)
        *out = *a1;
    else
        *out = *a2;
}

template <class T>
void
ConstInterp<T>::InterpArray(void *out_, void *a1_, void *a2_, int l, double f)
{
    T *out = (T*)out_;
    T *a1  = (T*)a1_;
    T *a2  = (T*)a2_;
    for (int i=0; i<l; i++)
    {
        if (f < .5)
            out[i] = a1[i];
        else
            out[i] = a2[i];
    }
}

template <class T>
void
ConstInterp<T>::InterpVector(void *out_, void *a1_, void *a2_, double f)
{
    vtkstd::vector<T> &out = *(vtkstd::vector<T>*)out_;
    vtkstd::vector<T> &a1  = *(vtkstd::vector<T>*)a1_;
    vtkstd::vector<T> &a2  = *(vtkstd::vector<T>*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 > l2)
        out = a1;
    else
        out = a2;
    int l = (l1 < l2) ? l1 : l2;
    for (int i=0; i<l; i++)
    {
        if (f < .5)
            out[i] = a1[i];
        else
            out[i] = a2[i];
    }
}

// ****************************************************************************
//  Methods:  ConstInterp<AttributeGroup*>::Interp*
//
//  Purpose:
//    Specialized methods for piecewise constant interpolation of Attribute
//    Group Vectors.
//
//  Arguments:
//    out        the pointer to the output location
//    a1         the pointer to the first  control point value
//    a2         the pointer to the second control point value
//    l          (where applicable) the length of the array
//    f          the alpha value from [0.0 , 1.0]
//
//  Note:  Assumes out.size is at least as great at the max of a1 & a2's size
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 23, 2003
//
//  Modifications:
//    Jeremy Meredith, Thu Feb 27 11:23:58 PST 2003
//    Added template<> to conform to new C++ rules.
//
// ****************************************************************************
template<>
void
ConstInterp<AttributeGroup*>::InterpVector(void *out_, void *a1_, void *a2_, double f)
{
    AttributeGroupVector &out= *(AttributeGroupVector*)out_;
    AttributeGroupVector &a1 = *(AttributeGroupVector*)a1_;
    AttributeGroupVector &a2 = *(AttributeGroupVector*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 > l2)
    {
        for (int i=l2; i<l1; i++)
            out[i]->CopyAttributes(a1[i]);
    }
    else
    {
        for (int i=l1; i<l2; i++)
            out[i]->CopyAttributes(a2[i]);
    }

    int l = (l1 < l2) ? l1 : l2;
    for (int i=0; i<l; i++)
    {
        if (f < .5)
            out[i]->CopyAttributes(a1[i]);
        else
            out[i]->CopyAttributes(a2[i]);
    }
}

// ****************************************************************************
//  Methods:  LinInterp<T>::Interp*
//
//  Purpose:
//    Methods for piecewise linear interpolation.
//
//  Arguments:
//    out        the pointer to the output location
//    a1         the pointer to the first  control point value
//    a2         the pointer to the second control point value
//    l          (where applicable) the length of the array
//    f          the alpha value from [0.0 , 1.0]
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 17, 2003
//
// ****************************************************************************
template <class T>
void
LinInterp<T>::InterpScalar(void *out_, void *a1_, void *a2_, double f)
{
    T *out = (T*)out_;
    T *a1  = (T*)a1_;
    T *a2  = (T*)a2_;
    *out = T((1.0 - f) * (*a1)  +  f * (*a2));
}

template <class T>
void
LinInterp<T>::InterpArray(void *out_, void *a1_, void *a2_, int l, double f)
{
    T *out = (T*)out_;
    T *a1  = (T*)a1_;
    T *a2  = (T*)a2_;
    for (int i=0; i<l; i++)
    {
        out[i] = T((1.0 - f) * (a1[i])  +  f * (a2[i]));
    }
}

template <class T>
void
LinInterp<T>::InterpVector(void *out_, void *a1_, void *a2_, double f)
{
    vtkstd::vector<T> &out = *(vtkstd::vector<T>*)out_;
    vtkstd::vector<T> &a1  = *(vtkstd::vector<T>*)a1_;
    vtkstd::vector<T> &a2  = *(vtkstd::vector<T>*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 > l2)
        out = a1;
    else
        out = a2;
    int l = (l1 < l2) ? l1 : l2;
    for (int i=0; i<l; i++)
        out[i] = T((1.0 - f) * (a1[i])  +  f * (a2[i]));
}

// ****************************************************************************
//  Methods:  LinInterp<int>::Interp*
//
//  Purpose:
//    Specialized methods for piecewise linear interpolation of integers.
//    Rounds to nearest integer after floating point interpolation.
//
//  Arguments:
//    out        the pointer to the output location
//    a1         the pointer to the first  control point value
//    a2         the pointer to the second control point value
//    l          (where applicable) the length of the array
//    f          the alpha value from [0.0 , 1.0]
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 17, 2003
//
//  Modifications:
//    Jeremy Meredith, Thu Feb 27 11:23:58 PST 2003
//    Added template<> to conform to new C++ rules.
//
// ****************************************************************************
template<>
void
LinInterp<int>::InterpScalar(void *out_, void *a1_, void *a2_, double f)
{
    int *out = (int*)out_;
    int *a1  = (int*)a1_;
    int *a2  = (int*)a2_;
    *out = int((1.0 - f) * (*a1)  +  f * (*a2)  + 0.5);  // round ints
}

template<>
void
LinInterp<int>::InterpArray(void *out_, void *a1_, void *a2_, int l, double f)
{
    int *out = (int*)out_;
    int *a1  = (int*)a1_;
    int *a2  = (int*)a2_;
    for (int i=0; i<l; i++)
        out[i] = int((1.0 - f) * (a1[i])  +  f * (a2[i])  + 0.5); // round ints
}

template<>
void
LinInterp<int>::InterpVector(void *out_, void *a1_, void *a2_, double f)
{
    vtkstd::vector<int> &out = *(vtkstd::vector<int>*)out_;
    vtkstd::vector<int> &a1  = *(vtkstd::vector<int>*)a1_;
    vtkstd::vector<int> &a2  = *(vtkstd::vector<int>*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 > l2)
        out = a1;
    else
        out = a2;
    int l = (l1 < l2) ? l1 : l2;
    for (int i=0; i<l; i++)
        out[i] = int((1.0 - f) * (a1[i])  +  f * (a2[i]));
}

// ****************************************************************************
//  Methods:  LinInterp<AttributeGroup*>::Interp*
//
//  Purpose:
//    Specialized methods for piecewise linear interpolation of Attribute
//    Group Vectors.
//
//  Arguments:
//    out        the pointer to the output location
//    a1         the pointer to the first  control point value
//    a2         the pointer to the second control point value
//    l          (where applicable) the length of the array
//    f          the alpha value from [0.0 , 1.0]
//
//  Note:  Assumes out.size is at least as great at the max of a1 & a2's size
//
//  Programmer:  Jeremy Meredith
//  Creation:    January 23, 2003
//
//  Modifications:
//    Jeremy Meredith, Thu Feb 27 11:23:58 PST 2003
//    Added template<> to conform to new C++ rules.
//
// ****************************************************************************
template<>
void
LinInterp<AttributeGroup*>::InterpVector(void *out_, void *a1_, void *a2_, double f)
{
    AttributeGroupVector &out= *(AttributeGroupVector*)out_;
    AttributeGroupVector &a1 = *(AttributeGroupVector*)a1_;
    AttributeGroupVector &a2 = *(AttributeGroupVector*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 > l2)
    {
        for (int i=l2; i<l1; i++)
            out[i]->CopyAttributes(a1[i]);
    }
    else
    {
        for (int i=l1; i<l2; i++)
            out[i]->CopyAttributes(a2[i]);
    }

    int l = (l1 < l2) ? l1 : l2;
    for (int i=0; i<l; i++)
    {
        out[i]->InterpolateLinear(a1[i],a2[i],f);
    }
}

#endif
