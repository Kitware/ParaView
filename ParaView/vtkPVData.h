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

#include "vtkPVActorComposite.h"
#include "vtkDataSet.h"
#include "vtkPVSourceCollection.h"

class vtkPVSource;
class vtkKWView;
class vtkPVApplication;
class vtkKWMenuButton;


class VTK_EXPORT vtkPVData : public vtkPVActorComposite
{
public:
  static vtkPVData* New();
  vtkTypeMacro(vtkPVData, vtkPVActorComposite);

  // Description:
  // This also creates the parallel vtk objects for the composite.
  // (actor, mapper, ...)
  void SetApplication(vtkPVApplication *pvApp);
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVData *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
  
  // Description:
  // DO NOT CALL THIS IF YOU ARE NOT A PVSOURCE!
  // The composite sets this so this data widget will know who owns it.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);
  
  // Description:
  // This is for setting up the links between VTK objects and PV object.
  // This call also sets the input to the mapper.
  // SetVTKData should be called after the application has been set, but before
  // PVData is used as input a filter or output of a source.
  // We could change the object so that it creates its own data (during initialization), 
  // but then we would have to tell it what type of data to create.
  void SetVTKData(vtkDataSet *data, const char *name);
  vtkGetObjectMacro(VTKData,vtkDataSet);  

  // Description:
  // The tcl name of the vtk data object.  This should be the primary method of 
  // manipulating the data since it exists on all processes.
  vtkGetStringMacro(VTKDataTclName);  
  
  // Description:
  // This method collects the bounds from all processes.
  // It expects the data to be up to date.
  void GetBounds(float bounds[6]);

  // Description:
  // This method collects the number of cells from all processes.
  // It expects the data to be up to date.
  int GetNumberOfCells();
  
  // Description:
  // This method collects the number of points from all processes.
  // It expects the data to be up to date.
  int GetNumberOfPoints();

  // Description:
  // We are keeping the forward links.  I have not 
  // considered deleting objects properly.
  // These methods are used internally. They are not meant to be called
  // by the user.
  void AddPVSourceToUsers(vtkPVSource *s);
  void RemovePVSourceFromUsers(vtkPVSource *s);
  vtkPVSourceCollection *GetPVSourceUsers() {return this->PVSourceCollection;}

  // Description:
  // This methiod updates the piece that has been assinged to this process.
  void Update();
  
protected:
  vtkPVData();
  ~vtkPVData();
  
  vtkPVData(const vtkPVData&) {};
  void operator=(const vtkPVData&) {};
  
  vtkDataSet *VTKData;
  char *VTKDataTclName;
  
  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;

  // Keep a list of sources that are using this data.
  // We may want to have a list that does not reference
  // count the sources.
  vtkPVSourceCollection *PVSourceCollection;
};

#endif
