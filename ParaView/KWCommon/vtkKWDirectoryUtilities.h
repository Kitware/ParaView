/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWDirectoryUtilities.h
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

#ifndef __vtkKWDirectoryUtilities_h
#define __vtkKWDirectoryUtilities_h

#include "vtkObject.h"

class VTK_EXPORT vtkKWDirectoryUtilities : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkKWDirectoryUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);  
  static vtkKWDirectoryUtilities* New();

  const char* GetEnv(const char* key);
  const char* GetCWD();
  const char* ConvertToUnixSlashes(const char* path);
  int FileExists(const char* filename);
  int FileIsDirectory(const char* name);
  char** GetSystemPath();
  const char* FindProgram(const char* name);
  const char* CollapseDirectory(const char* dir);
  const char* FindSelfPath(const char* argv0);
  
protected:
  vtkKWDirectoryUtilities();
  ~vtkKWDirectoryUtilities();
  
  char** SystemPath;
  char* CWD;
  char* UnixSlashes;
  char* ProgramFound;
  char* SelfPath;
  char* CollapsedDirectory;
  
private:
  vtkKWDirectoryUtilities(const vtkKWDirectoryUtilities&); // Not implemented
  void operator=(const vtkKWDirectoryUtilities&); // Not implemented
};

#endif
