/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSource.h
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
// .NAME vtkPVSource - A super class for filter objects.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.
// This class should probably be merged with vtkPVComposite.


#ifndef __vtkPVSource_h
#define __vtkPVSource_h

#include "vtkKWLabel.h"
#include "vtkPVData.h"
#include "vtkSource.h"

class vtkPVComposite;
class vtkPVAssignment;

class VTK_EXPORT vtkPVSource : public vtkKWWidget
{
public:
  static vtkPVSource* New();
  vtkTypeMacro(vtkPVSource,vtkKWWidget);
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.
  void Clone(vtkPVApplication *app);
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  virtual int Create(char *args);  
  
  // Description:
  // DO NOT CALL THIS IF YOU ARE NOT A COMPOSITE!
  void SetComposite(vtkPVComposite *comp);

  // Description:
  // Gets the composite that owns this source widget.
  vtkGetObjectMacro(Composite, vtkPVComposite);

  // Description:
  // Tells the filter which piece of data to generate.
  // Makes the call (indirectly) in parallel (on every process).
  virtual void SetAssignment(vtkPVAssignment *a);
  
  // Description:
  // A way to get the output in the superclass.
  vtkPVData *GetPVData() {return this->Output;}
    
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
protected:
  vtkPVSource();
  ~vtkPVSource();
  vtkPVSource(const vtkPVSource&) {};
  void operator=(const vtkPVSource&) {};
  
  // Description:
  // Mangages the double pointer and reference counting.
  void SetPVData(vtkPVData *data);
  
  vtkPVData *Output;
  
  // Just one input for now.
  vtkPVData *Input;

  vtkPVComposite* Composite;
  
};

#endif


