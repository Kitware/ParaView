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
class vtkPVScalarBar;
class vtkPVSource;
class vtkPVAssignment;
class vtkPVApplication;
class vtkKWMenuButton;


class VTK_EXPORT vtkPVData : public vtkKWWidget
{
public:
  static vtkPVData* New();
  vtkTypeMacro(vtkPVData, vtkKWWidget);
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  virtual vtkPVData *MakeObject(){vtkErrorMacro("No MakeObject");return NULL;}
  
  // Description:
  // This duplicates the object in the satellite processes.
  // They will all have the same tcl name.
  void Clone(vtkPVApplication *app);
  
  // Description:
  // Creates common widgets.
  // Returns 0 if there was an error.
  virtual int Create(char *args);
  
  // Description:
  // This is a parallel method.  All out satellite datas will get the
  // equivalent actor composite. The main reason this is here is to allow 
  // the actor composite to be created in the Clone method rather than
  // the constructor.
  void SetActorComposite(vtkPVActorComposite *c);
  
  // Description:
  // This composite actually has an actor that displays the data.
  vtkPVActorComposite* GetActorComposite();
  
  // Description:
  // Set/Get the scalar bar associated with this data object.
  void SetPVScalarBar(vtkPVScalarBar *sb);
  vtkPVScalarBar *GetPVScalarBar();
                    
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
  // Like update extent, but an object tells which piece to assign this process.
  virtual void SetAssignment(vtkPVAssignment *a);
  vtkPVAssignment *GetAssignment() {return this->Assignment;}
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // A generic way of getting the data.
  vtkGetObjectMacro(Data,vtkDataSet);

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
  vtkSetObjectMacro(Data, vtkDataSet);  
  
  void ShowActorComposite();
  void ShowScalarBarParameters();
  
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
  vtkDataSet *Data;

  vtkPVActorComposite *ActorComposite;
  vtkKWPushButton *ActorCompositeButton;
  vtkPVScalarBar *PVScalarBar;
  vtkKWPushButton *ScalarBarButton;
  
  // This points to the source widget that owns this data widget.
  vtkPVSource *PVSource;
  vtkPVAssignment *Assignment;

  // Keep a list of sources that are using this data.
  // We may want to have a list that does not reference
  // count the sources.
  vtkPVSourceCollection *PVSourceCollection;
};

#endif
