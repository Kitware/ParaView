/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWActorComposite.h
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
// .NAME vtkKWActorComposite - a composite for actors
// .SECTION Description
// A composite designed for actors. The actor has a vtkPolyDataMapper as
// a mapper, and the user specifies vtkPolyData as the input of this 
// composite.

#ifndef __vtkKWActorComposite_h
#define __vtkKWActorComposite_h

#include "vtkKWComposite.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkKWRadioButton.h"
#include "vtkKWOptionMenu.h"
class vtkKWApplication;
class vtkKWView;
class vtkPolyData;

class VTK_EXPORT vtkKWActorComposite : public vtkKWComposite
{
public:
  static vtkKWActorComposite* New();
  vtkTypeMacro(vtkKWActorComposite,vtkKWComposite);

  // Description:
  // Create the properties object, called by UpdateProperties.
  virtual void CreateProperties();

  // Description:
  // Set the input data for this Actor
  virtual void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput() {return this->Mapper->GetInput();};
  
  // Description:
  // Get the prop for this composite
  virtual vtkProp *GetProp() {return this->Actor;};
  virtual vtkActor *GetActor() {return this->Actor;};
  
protected:
  vtkKWActorComposite();
  ~vtkKWActorComposite();
  vtkKWActorComposite(const vtkKWActorComposite&) {};
  void operator=(const vtkKWActorComposite&) {};

  vtkActor *Actor;
  vtkPolyDataMapper *Mapper;
};


#endif
