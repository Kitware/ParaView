/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerXDMFParameters.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

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
// .NAME vtkPVServerXDMFParameters - Server-side helper for vtkPVArraySelection.
// .SECTION Description

#ifndef __vtkPVServerXDMFParameters_h
#define __vtkPVServerXDMFParameters_h

#include "vtkPVServerObject.h"

class vtkClientServerStream;
class vtkPVServerXDMFParametersInternals;
class vtkXdmfReader;

class VTK_EXPORT vtkPVServerXDMFParameters : public vtkPVServerObject
{
public:
  static vtkPVServerXDMFParameters* New();
  vtkTypeRevisionMacro(vtkPVServerXDMFParameters, vtkPVServerObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the set of parameters that may be specified for the given
  // XDMF reader.
  const vtkClientServerStream& GetParameters(vtkXdmfReader*);

protected:
  vtkPVServerXDMFParameters();
  ~vtkPVServerXDMFParameters();

  vtkPVServerXDMFParametersInternals* Internal;
private:
  vtkPVServerXDMFParameters(const vtkPVServerXDMFParameters&); // Not implemented
  void operator=(const vtkPVServerXDMFParameters&); // Not implemented
};

#endif
