/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVActorComposite.h
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
// .NAME vtkPVActorComposite - a composite for actors
// .SECTION Description
// A composite designed for actors. The actor has a vtkPolyDataMapper as
// a mapper, and the user specifies vtkPolyData as the input of this 
// composite.

#ifndef __vtkPVActorComposite_h
#define __vtkPVActorComposite_h

#include "vtkKWActorComposite.h"
#include "vtkKWLabel.h"
#include "vtkPVApplication.h"
#include "vtkDataSetMapper.h"

class vtkPVAssignment;
class vtkPVApplication;


class VTK_EXPORT vtkPVActorComposite : public vtkKWActorComposite
{
public:
  static vtkPVActorComposite* New();
  vtkTypeMacro(vtkPVActorComposite, vtkKWActorComposite);

  // Description:
  // This is a parallel object.  A clone will exist in every process.
  // After the object is constructed, clone should be called to
  /// create duplicate objects with the same tcl name.
  void Clone(vtkPVApplication *pvApp);  
  
  // Description:
  // Create the properties object, called by UpdateProperties.
  void CreateProperties();

  void ShowProperties();
  
  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetName(const char *name);
  char* GetName();
  
  void Select(vtkKWView *v);
  void Deselect(vtkKWView *v);
  
  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);
    
  // Description:
  // The mapper needs to know what the assignment is.
  void SetAssignment(vtkPVAssignment *a);

  // Description:
  // Parallel methods for computing the scalar range from the input,
  /// and setting the scalar range of the mapper.
  void GetInputScalarRange(float range[2]);
  void TransmitInputScalarRange();
  void SetScalarRange(float min, float max);
    
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);
  
protected:
  vtkPVActorComposite();
  ~vtkPVActorComposite();
  vtkPVActorComposite(const vtkPVActorComposite&) {};
  void operator=(const vtkPVActorComposite&) {};
  
  vtkKWWidget *Properties;
  vtkKWLabel *Label;
  char *Name;
};

#endif
