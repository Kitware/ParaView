/*=========================================================================

  Program:   ParaView
  Module:    vtkSingleContourFilter.h
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
// .NAME vtkSingleContourFilter - One contour for easy UI.
// .SECTION Description
// We may have a way to add contours in the future, 
// but now you can have only one.

#ifndef __vtkSingleContourFilter_h
#define __vtkSingleContourFilter_h

#include "vtkKitwareContourFilter.h"


class VTK_EXPORT vtkSingleContourFilter : public vtkKitwareContourFilter
{
public:
  static vtkSingleContourFilter* New();
  vtkTypeMacro(vtkSingleContourFilter, vtkKitwareContourFilter);

  // Description:
  // Add value would not work.  We need a simple interface.
  void SetFirstValue(float val);
  float GetFirstValue();

protected:
  vtkSingleContourFilter() {};
  ~vtkSingleContourFilter() {};
  vtkSingleContourFilter(const vtkSingleContourFilter&) {};
  void operator=(const vtkSingleContourFilter&) {};
};

#endif
