/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWExtent.h
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
// .NAME vtkKWExtent - six sliders defining a (xmin,xmax,ymin,ymax,zmin,zmax) extent
// .SECTION Description
// vtkKWExtent is a widget containing six sliders which represent the
// xmin, xmax, ymin, ymax, zmin, zmax extent of a volume. It is a 
// convinience object and has logic to keep the min values less than
// or equal to the max values.

#ifndef __vtkKWExtent_h
#define __vtkKWExtent_h

#include "vtkKWScale.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWExtent : public vtkKWWidget
{
public:
  static vtkKWExtent* New();
  vtkTypeMacro(vtkKWExtent,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the Range of the Extent, this is the range of
  // acceptable values for the sliders. Specified as 
  // minx maxx miny maxy minz maxz
  void SetExtentRange(float *);
  void SetExtentRange(float,float,float,float,float,float);

  
  // Description:
  // Set/Get the Extent.
  vtkGetVector6Macro(Extent,float);
  void SetExtent(float *);
  void SetExtent(float,float,float,float,float,float);

  // Description:
  // handle the callback, this is called internally when one of the 
  // sliders has been moved.
  void ExtentSelected();

  vtkKWScale *GetXMinScale() { return this->XMinScale; };
  vtkKWScale *GetXMaxScale() { return this->XMaxScale; };
  vtkKWScale *GetYMinScale() { return this->YMinScale; };
  vtkKWScale *GetYMaxScale() { return this->YMaxScale; };
  vtkKWScale *GetZMinScale() { return this->ZMinScale; };
  vtkKWScale *GetZMaxScale() { return this->ZMaxScale; };

  // Description:
  // A method to set callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The calling is done via TCL wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char *MethodAndArgString);

protected:
  vtkKWExtent();
  ~vtkKWExtent();
  vtkKWExtent(const vtkKWExtent&) {};
  void operator=(const vtkKWExtent&) {};

  char *Command;
  float Extent[6];
  vtkKWScale  *XMinScale;
  vtkKWScale  *XMaxScale;
  vtkKWScale  *YMinScale;
  vtkKWScale  *YMaxScale;
  vtkKWScale  *ZMinScale;
  vtkKWScale  *ZMaxScale;
};


#endif


