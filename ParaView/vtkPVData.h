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

#include "vtkDataSet.h"
#include "vtkKWPushButton.h"
#include "vtkPVSourceCollection.h"

class vtkPVActorComposite;
class vtkPVSource;
class vtkPVApplication;
class vtkKWMenuButton;


class VTK_EXPORT vtkPVData : public vtkKWWidget
{
public:
  static vtkPVData* New();
  vtkTypeMacro(vtkPVData, vtkKWWidget);

  // Description:
  // Call this after the object has been constructed.  We need this
  // because the constructor does not take an argument.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVData *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  virtual int Create(char *args);
  
  // Description:
  // This composite actually has an actor that displays the data.
  vtkPVActorComposite* GetActorComposite();
  
  // Description:
  // General filters that can be applied to vtkDataSet.
  void Contour();
  void ColorByProcess();
  void Elevation();
  void ExtractEdges();
  void Cutter();

  // Description:
  // DO NOT CALL THIS IF YOU ARE NOT A PVSOURCE!
  // The composite sets this so this data widget will know who owns it.
  void SetPVSource(vtkPVSource *source);
  vtkGetObjectMacro(PVSource, vtkPVSource);
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // A generic way of getting the data.
  vtkGetObjectMacro(VTKData,vtkDataSet);

  // Description:
  // Uses the assignment to set the extent, then updates the data.
  virtual void Update();

  // Description:
  // This method collects the bounds from all processes.
  void GetBounds(float bounds[6]);

  // Description:
  // A Helper method for GetBounds that needs to be wrapped.
  // Do not call this method directly.
  void TransmitBounds();
  
  // Description:
  // This method collects the number of cells from all processes.
  int GetNumberOfCells();
  
  // Description:
  // A Helper method for GetNumberOfCells that needs to be wrapped.
  // Do not call this method directly.
  void TransmitNumberOfCells();
  
  // Description:
  // This is for setting up the links between VTK objects and PV object.
  // Subclasses overide this method so that they can create special
  // mappers for the actor composites.  The user should not call this method.
  vtkSetObjectMacro(VTKData, vtkDataSet);  
  
  // Description:
  // The tcl name of the vtk data object.
  vtkGetStringMacro(VTKDataTclName);  
  
  void ShowActorComposite();
  
  // Description:
  // We are keeping the forward links.  I have not 
  // considered deleting objects properly.
  // These methods are used internally. They are not meant to be called
  // by the user.
  void AddPVSourceToUsers(vtkPVSource *s);
  void RemovePVSourceFromUsers(vtkPVSource *s);
  vtkPVSourceCollection *GetPVSourceUsers() {return this->PVSourceCollection;}

protected:
  vtkPVData();
  ~vtkPVData();
  
  vtkPVData(const vtkPVData&) {};
  void operator=(const vtkPVData&) {};
  
  vtkKWMenuButton *FiltersMenuButton;
  vtkDataSet *VTKData;
  char *VTKDataTclName;
  vtkSetStringMacro(VTKDataTclName);
  
  vtkPVActorComposite *ActorComposite;
  vtkKWPushButton *ActorCompositeButton;
  
  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;

  // Keep a list of sources that are using this data.
  // We may want to have a list that does not reference
  // count the sources.
  vtkPVSourceCollection *PVSourceCollection;
};

#endif
