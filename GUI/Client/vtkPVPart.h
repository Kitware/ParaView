/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPart.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPart - PVDatas are now composed of multiple parts.
// .SECTION Description
// This object manages one vtk data set and all the vtk objects
// necessary to render the data set.
// The class has no user iterface.
// For now:  It is a PV object and resides only on process 0.
// Since it has no user interface, it we might want it on all process.

#ifndef __vtkPVPart_h
#define __vtkPVPart_h


#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed For Set Get VTKDataID

class vtkCollection;
class vtkDataSet;
class vtkPVProcessModule;
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkPVPartDisplay;
class vtkPVDisplay;
class vtkPolyDataMapper;
class vtkSMPart;

class VTK_EXPORT vtkPVPart : public vtkObject
{
public:
  static vtkPVPart* New();
  vtkTypeRevisionMacro(vtkPVPart, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // This also creates the parallel vtk objects for the composite.
  // (actor, mapper, ...)
  void SetProcessModule(vtkPVProcessModule *pm);

  // Description:
  // Set the id for the vtk data object.  This should be the primary
  // method of manipulating the data since it exists on all processes.
  // This is for setting up the links between VTK objects and PV
  // object.  This call also sets the input to the mapper.
  // SetVTKDataTclName should be called after the application has been
  // set, but befor PVData is used as input a filter or output of a
  // source.  We could change the object so that it creates its own
  // data (during initia but then we would have to tell it what type
  // of data to create.
  vtkClientServerID GetVTKDataID();
  
  //===================
          
  // Description:
  // Casts to vtkPVApplication.
  vtkPVProcessModule *GetProcessModule() { return this->ProcessModule;}
  
  // Description:
  // Moving away from direct access to VTK data objects.
  vtkPVDataInformation* GetDataInformation();
  vtkGetObjectMacro(ClassNameInformation, vtkPVClassNameInformation);
  //ETX
  
  // Description:
  // This method collects data information from all processes.
  // This needs to be called before this parts information
  // is valid.
  void GatherDataInformation();

  // Description:
  // The name is just a string that will be used in the extract part UI.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Update the data and geometry.
  void Update();

  // Description:
  // Modified propagated forward to eliminate extra network update calls.
  void MarkForUpdate();

  // Description:
  // Set visibility of all of the displays.
  // I would like to remove this method once the properties are finished.
  // UI would directly manipulate the displays.
  void SetVisibility(int v);

//BTX
  // Description:
  // vtkPVPart and vtkSMPart will be merged in the future 
  // (vtkPVPart will go away).
  void SetSMPart(vtkSMPart* smpart);
  vtkSMPart* GetSMPart() {return this->SMPart;}

  // Description:
  // Temporary access to the display object.
  // Render modules may eleimnate the need for this access.
  vtkPVPartDisplay* GetPartDisplay();
  void SetPartDisplay(vtkPVPartDisplay* pDisp);

  // Description:
  // We are starting to support multiple types of displays (plot).
  // I am keeping the PartDisplay pointer and methods around
  // until we come up with a better API (maybe proxy/properties).
  // The method SetPartDisplay also adds the display to this collection.
  void AddDisplay(vtkPVDisplay* disp);
//ETX

protected:
  vtkPVPart();
  ~vtkPVPart();

  // A part needs a name to show in the extract part filter.
  // We are also going to allow expresion matching.
  char *Name;

  vtkPVClassNameInformation *ClassNameInformation;
  
//BTX  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVProcessModule *pm);
//ETX

  // If the data changes, we need to change to.
  vtkTimeStamp UpdateTime;

  vtkSMPart* SMPart;
  vtkPVProcessModule* ProcessModule;

private:
  vtkPVPart(const vtkPVPart&); // Not implemented
  void operator=(const vtkPVPart&); // Not implemented
};

#endif
