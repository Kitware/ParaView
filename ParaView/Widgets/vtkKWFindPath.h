/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWFindPath.h
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
// .NAME vtkKWFindPath - helper class for finding paths

#ifndef __vtkKWFindPath_h
#define __vtkKWFindPath_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWFindPath : public vtkKWObject
{
public:
  static vtkKWFindPath* New();
  vtkTypeMacro(vtkKWFindPath,vtkKWObject);

  // Description:
  // Initialize Find path. The argument is the argv[0] from main.
  void Initialize(const char* argv0);
  
  // Description:
  // Find directory relative to the executable.
  // The path is available until next call, so 
  // you do not have to delete string.
  const char* FindDirectory(const char* directory);

  // Description:
  // Get executable name and path.
  vtkGetStringMacro(ExecutableName);
  vtkGetStringMacro(ExecutablePath);

  // Description:
  // Get the current working directory.
  vtkGetStringMacro(CurrentWorkingDirectory);

  // Description:
  // Change current directory.
  int ChangeDirectory(const char* newdir);
  
  // Description:
  // Get absolute path from the relative path.
  // The path is available until next call, so 
  // you do not have to delete string.
  const char* GetAbsolutePath(const char* path);

protected:
  // Description:
  // Set methods for internal strings.
  vtkSetStringMacro(ExecutableName);
  vtkSetStringMacro(ExecutablePath);
  vtkSetStringMacro(AbsolutePath);
  vtkSetStringMacro(CurrentWorkingDirectory);
  vtkSetStringMacro(LastFindPath);

  vtkKWFindPath();
  ~vtkKWFindPath();

  vtkKWFindPath(const vtkKWFindPath&) {};
  void operator=(const vtkKWFindPath&) {};
  
  char *ExecutableName;
  char *ExecutablePath;
  char *AbsolutePath;
  char *LastFindPath;
  char *CurrentWorkingDirectory;
};


#endif


