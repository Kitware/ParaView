/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoDistillerSTD.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoDistillerSTD.h

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
// .NAME vtkCosmoDistillerSTD - find halos within a cosmology data file
// .SECTION Description
// vtkCosmoDistillerSTD is a filter object that operates on two unstructured 
// grids created with two vtkCosmoReaderHPAR reads for two particle files.
// It assigns the halotag of all particles in the second file based on the
// halotag assignment of the first file. 
//
// .NOTE
// follows the majority rule in the halotag assignement.
//

#ifndef __vtkCosmoDistillerSTD_h
#define __vtkCosmoDistillerSTD_h

#include "vtkUnstructuredGridAlgorithm.h"
class vtkAlgorithmOutput;

class VTK_EXPORT vtkCosmoDistillerSTD : public vtkUnstructuredGridAlgorithm
{
 public:
  static vtkCosmoDistillerSTD *New();

  vtkTypeRevisionMacro(vtkCosmoDistillerSTD,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // equivalent to SetInputConnection(1, algOutput)
  // this function is provide for configuring the second input port in the XML file
  void SetSourceConnection(vtkAlgorithmOutput *algOutput);
  
  // equivalent to SetInputArrayToProcess(1, 1, connection, fieldAssociation, name)
  // this function is provide for configuring the second input port in the XML file
  void SetSourceArrayToProcess(int idx, int port, int connection, int fieldAssociation, const char *name);

 protected:

  vtkCosmoDistillerSTD();
  ~vtkCosmoDistillerSTD();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

 private:
  vtkCosmoDistillerSTD(const vtkCosmoDistillerSTD&);  // Not implemented.
  void operator=(const vtkCosmoDistillerSTD&);  // Not implemented.
};

#endif //  __vtkCosmoDistillerSTD_h
