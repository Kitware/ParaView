/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSerializer.h
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
// .NAME vtkKWSerializer - a helper class for serialization
// .SECTION Description
// vtkKWSerializer is a helper class that is used by objects to 
// serialize themselves. Put another way, it helps instances write
// or read themselves from disk.

#ifndef __vtkKWSerializer_h
#define __vtkKWSerializer_h

#include "vtkKWObject.h"

// var args
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"

class VTK_EXPORT vtkKWSerializer : public vtkObject
{
public:
  static vtkKWSerializer* New();
  vtkTypeMacro(vtkKWSerializer,vtkObject);

  // Description:
  // The primary helper functions instances can invoke.
  static void FindClosingBrace(istream *is, vtkObject *obj);
  static void ReadNextToken(istream *is,const char *tok, vtkObject *obj);
  static int GetNextToken(istream *is, char result[1024]);
  static void WriteSafeString(ostream& os, const char *val);
  
protected:
  vtkKWSerializer() {};
  ~vtkKWSerializer() {};
  vtkKWSerializer(const vtkKWSerializer&) {};
  void operator=(const vtkKWSerializer&) {};

  static void EatWhiteSpace(istream *is);
};


#endif


