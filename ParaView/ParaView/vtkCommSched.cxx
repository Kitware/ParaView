/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommSched.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/*======================================================================
// This software and ancillary information known as vtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.
======================================================================*/


#include "vtkCommSched.h"
#include "vtkObjectFactory.h"

//*****************************************************************

vtkCommSched* vtkCommSched::New()
{

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCommSched");
  if(ret)
    {
    return (vtkCommSched*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCommSched;
}
//*****************************************************************

vtkCommSched::vtkCommSched()
{
  // ... initalize a communication schedule to do nothing ...
  numCells = 0;
  cntSend  = 0;
  cntRec   = 0;
  sendTo  = NULL;
  sendNum = NULL;
  recFrom = NULL;
  recNum  = NULL;
  sendCellList = NULL;
  keepCellList = NULL;
}
//*****************************************************************
vtkCommSched::~vtkCommSched()
{
  if (sendTo  != NULL) delete [] sendTo;
  if (sendNum != NULL) delete [] sendNum;
  if (recFrom != NULL) delete [] recFrom;
  if (recNum  != NULL) delete [] recNum;
  if (sendCellList != NULL) 
  {
     for (int i=0; i<cntSend; i++) delete [] sendCellList[i];
     delete [] sendCellList;
  }
  if (keepCellList != NULL) delete [] keepCellList; 
}
#if 0
//*****************************************************************
// ... copy constructor ...
vtkCommSched::vtkCommSched(vtkCommSched& sched)
{
  numCells = sched.numCells;
  cntSend  = sched.cntSend;
  cntRec   = sched.cntRec;
  sendTo  = new int [cntSend];
  sendNum = new vtkIdType [cntSend];
  int i;
  vtkIdType j;
  for (i=0; i<cntSend; i++)
    {
    sendTo[i]  = sched.sendTo[i];
    sendNum[i] = sched.sendNum[i];
    }
  if (sched.sendCellList != NULL)
    {
    sendCellList = new vtkIdType*[cntSend];
    for (i=0; i<cntSend; i++)
      {
      sendCellList[i] = new vtkIdType[sendNum[i]];
      for (j=0; j<sendNum[i]; j++)
	sendCellList[i][j] = sched.sendCellList[i][j];
      }
    }
  recFrom = new int [cntRec];
  recNum  = new vtkIdType [cntRec];
  for (i=0; i<cntRec; i++)
    {
    recFrom[i] = sched.recFrom[i];
    recNum[i]  = sched.recNum[i];
    }
  if (sched.keepCellList != NULL)
    {
    keepCellList = new vtkIdType [numCells];
    for (i=0; i<numCells; i++) keepCellList[i] = sched.keepCellList[i];
    }
  
}
//*****************************************************************
#endif
void vtkCommSched::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  int i;
  os << indent << "numCells (" << this->numCells<< ")/n";

  os << indent << "cntSend (" << this->cntSend<< ")/n";
  for (i=0; i<cntSend; i++)
  {
    os << indent << "sendTo["<<i<<"] (" << this->sendTo[i]<< ")/n";
    os << indent << "sendNum["<<i<<"] (" << this->sendNum[i]<< ")/n";
  }

  os << indent << "cntRec (" << this->cntRec<< ")/n";
  for (i=0; i<cntRec; i++)
  {
    os << indent << "recFrom["<<i<<"] (" << this->recFrom[i]<< ")/n";
    os << indent << "recNum["<<i<<"] (" << this->recNum[i]<< ")/n";
  }

}


