/*=========================================================================

  Program:   ParaView
  Module:    vtkStringList.h
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
// .NAME vtkKWAssignment - An object for assigning data to processors.
// .SECTION Description
// A vtkStringList holds a data piece/extent specification for a process.
// It is really just a parallel object wrapper for vtkPVExtentTranslator.

#ifndef __vtkStringList_h
#define __vtkStringList_h

#include "vtkObject.h"


class VTK_EXPORT vtkStringList : public vtkObject
{
public:
  static vtkStringList* New();
  vtkTypeMacro(vtkStringList,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
//BTX
  // Description:
  // Add a command and format it any way you like/
  void AddString(const char *EventString, ...);
//ETX
  
  // Description:
  // Initialize to empty.
  void RemoveAllItems();

  // Description:
  // Random access.
  void SetString(int idx, const char *str);

  // Description:
  // Get the length of the list.
  int GetLength() { return this->NumberOfStrings;}
  
  // Description:
  // Get a command from its index.
  char *GetString(int idx);
  
  vtkGetMacro(NumberOfStrings, int);
  
protected:
  vtkStringList();
  ~vtkStringList();
  
  int NumberOfStrings;
  int StringArrayLength;
  char **Strings;
  void Reallocate(int num);
  void DeleteStrings();

  vtkStringList(const vtkStringList&); // Not implemented
  void operator=(const vtkStringList&); // Not implemented
};

#endif
