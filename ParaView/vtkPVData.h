/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVData.h
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

#ifndef __vtkPVData_h
#define __vtkPVData_h

#include "vtkProp.h"
#include "vtkDataSetMapper.h"
#include "vtkPVMenuButton.h"
#include "vtkActor.h"
#include "vtkDataSet.h"

class vtkPVSource;
class vtkPVAssignment;
class vtkPVApplication;


class VTK_EXPORT vtkPVData : public vtkKWWidget
{
public:
  static vtkPVData* New();
  vtkTypeMacro(vtkPVData, vtkKWWidget);
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.
  void Clone(vtkPVApplication *app);
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  virtual int Create(char *args);
  
  vtkProp* GetProp();
  
  // Description:
  // General filters that can be applied to vtkDataSet.
  void Contour();

  // Description:
  // DO NOT CALL THIS IF YOU ARE NOT A COMPOSITE!
  // The composite sets this so this data widget will know who owns it.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);
  
  // Description:
  // Like update extent, but an object tells which piece to assign this process.
  virtual void SetAssignment(vtkPVAssignment *a);
  vtkPVAssignment *GetAssignment() {return this->Assignment;}
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // A generic way of getting the data.
  vtkGetObjectMacro(Data,vtkDataSet);
  
protected:
  vtkPVData();
  ~vtkPVData();
  
  vtkPVData(const vtkPVData&) {};
  void operator=(const vtkPVData&) {};
  
  vtkSetObjectMacro(Data, vtkDataSet);

  vtkPVMenuButton *FiltersMenuButton;
  vtkDataSet *Data;
  vtkDataSetMapper *Mapper;
  vtkActor *Actor;

  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;
  vtkPVAssignment *Assignment;
};

#endif

