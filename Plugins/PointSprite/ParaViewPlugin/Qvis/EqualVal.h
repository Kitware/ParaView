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

#ifndef EQUALVAL_H
#define EQUALVAL_H

#include <vector>

// ****************************************************************************
//  Class:  EqualVal 
//
//  Purpose:
//    Comparison functions
//
//  Programmer:  Mark C. Miller 
//  Creation:    06May03 
//
// ****************************************************************************
template <class T>
struct EqualVal
{
    inline static bool EqualScalar(void *a1,void *a2);
    inline static bool EqualArray (void *a1,void *a2, int l);
    inline static bool EqualVector(void *a1,void *a2);
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//                               Inline Methods
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


// ****************************************************************************
//  Methods:  EqualVal<T>::Equal*
//
//  Purpose:
//    Methods for comparison.
//
//  Arguments:
//    a1         the pointer to the first operand of comparison
//    a2         the pointer to the second operand of comparison 
//    l          (where applicable) the length of the array
//
//  Programmer:  Mark C. Miller (cut/paste/modify from Jeremey's stuff)
//  Creation:    06May03 
//
// ****************************************************************************
template <class T>
bool
EqualVal<T>::EqualScalar(void *a1_, void *a2_)
{
    T *a1  = (T*)a1_;
    T *a2  = (T*)a2_;
    if (*a1 == *a2)
       return true;
    else
       return false;
}

template <class T>
bool
EqualVal<T>::EqualArray(void *a1_, void *a2_, int l)
{
    if (a1_ == a2_)
       return true;
    T *a1  = (T*)a1_;
    T *a2  = (T*)a2_;
    for (int i=0; i<l; i++)
    {
       if (a1[i] != a2[i])
          return false;
    }
    return true;
}

template <class T>
bool
EqualVal<T>::EqualVector(void *a1_, void *a2_)
{
    if (a1_ == a2_)
       return true;
    vtkstd::vector<T> &a1  = *(vtkstd::vector<T>*)a1_;
    vtkstd::vector<T> &a2  = *(vtkstd::vector<T>*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 != l2)
        return false;
    else
    {
        for (int i=0; i<l1; i++)
        {
           if (a1[i] != a2[i])
              return false;
        }
    }
    return true;
}

// ****************************************************************************
//  Method:  EqualVal<AttributeGroup*>::EqualVector
//
//  Purpose:
//    Specialized method for equality of Attribute Group Vectors.
//
//  Arguments:
//    a1         the pointer to the first  operand of comparsion 
//    a2         the pointer to the second operand of comparison 
//
//  Programmer:  Mark C. Miller 
//  Creation:    06May03 
//
// ****************************************************************************
template<>
bool
EqualVal<AttributeGroup*>::EqualVector(void *a1_, void *a2_)
{
    if (a1_ == a2_)
       return true;
    AttributeGroupVector &a1 = *(AttributeGroupVector*)a1_;
    AttributeGroupVector &a2 = *(AttributeGroupVector*)a2_;
    int l1 = static_cast<int>(a1.size());
    int l2 = static_cast<int>(a2.size());
    if (l1 != l2)
       return false;
    for (int i=0; i<l1; i++)
    {
        if (!a1[i]->EqualTo(a2[i]))
           return false;
    }
    return true;
}

#endif
