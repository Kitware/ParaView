/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoCorrelater.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoCorrelater.h

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCosmoCorrelater - find halos within a cosmology data file
// .SECTION Description
// vtkCosmoCorrelater is a filter object that operates on two unstructured 
// grids created with two vtkCosmoReaderHPAR reads for two particle files.
// It assigns the halotag of all particles in the second file based on the
// halotag assignment of the first file. 
//
// .NOTE
// follows the majority rule in the halotag assignement.
//

#ifndef __vtkCosmoCorrelater_h
#define __vtkCosmoCorrelater_h

#include "vtkUnstructuredGridAlgorithm.h"
#include <vtkstd/string>

class VTK_EXPORT vtkCosmoCorrelater : public vtkUnstructuredGridAlgorithm
{
 public:
  // Description:
  static vtkCosmoCorrelater *New();

  vtkTypeRevisionMacro(vtkCosmoCorrelater,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(np,int);
  vtkGetMacro(np,int);

  vtkSetMacro(bb,float);
  vtkGetMacro(bb,float);

  vtkSetMacro(rL,float);
  vtkGetMacro(rL,float);

  vtkSetMacro(Periodic,bool);
  vtkGetMacro(Periodic,bool);

  void SetFieldName(const char* field_name);

  void SetQueryConnection(vtkAlgorithmOutput *algOutput);
 protected:
  // input 
  int np;
  float bb;
  float rL;
  bool Periodic;

  vtkCosmoCorrelater();
  ~vtkCosmoCorrelater();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

 private:

  vtkCosmoCorrelater(const vtkCosmoCorrelater&);  // Not implemented.
  void operator=(const vtkCosmoCorrelater&);  // Not implemented.

  // 3d-tree
  int *seq;
  long long *v;
  float **data;
  float *mediantest;
  void Reorder(long long *, long long *, int);

  // range search
  float *range;
  int nmap;
  void RangeSearch(int, int, int, float *);

  //BTX
  class vtkInternal;
  vtkInternal* Internal;
  //ETX

};

#endif //  __vtkCosmoCorrelater_h
