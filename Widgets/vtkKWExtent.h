/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWExtent.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkKWExtent
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
  vtkKWExtent();
  ~vtkKWExtent();
  static vtkKWExtent* New();
  const char *GetClassName() {return "vtkKWExtent";};

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // Set the Range of the Extent, this is the range of
  // acceptable values for the sliders. Specified as 
  // minx maxx miny maxy minz maxz
  void SetExtentRange(int *);
  void SetExtentRange(int,int,int,int,int,int);

  
  // Description:
  // Set/Get the Extent.
  vtkGetVector6Macro(Extent,int);
  void SetExtent(int *);
  void SetExtent(int,int,int,int,int,int);

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
  char *Command;
  int Extent[6];
  vtkKWScale  *XMinScale;
  vtkKWScale  *XMaxScale;
  vtkKWScale  *YMinScale;
  vtkKWScale  *YMaxScale;
  vtkKWScale  *ZMinScale;
  vtkKWScale  *ZMaxScale;
};


#endif


