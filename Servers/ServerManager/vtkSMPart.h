/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPart.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPart - PVDatas are now composed of multiple parts.
// .SECTION Description
// This object manages one vtk data set and all the vtk objects
// necessary to render the data set.
// The class has no user iterface.
// For now:  It is a PV object and resides only on process 0.
// Since it has no user interface, it we might want it on all process.

#ifndef __vtkSMPart_h
#define __vtkSMPart_h


#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed For Set Get VTKDataID

class vtkDataSet;
class vtkPVApplication;
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkSMPartDisplay;
class vtkPolyDataMapper;

class VTK_EXPORT vtkSMPart : public vtkObject
{
public:
  static vtkSMPart* New();
  vtkTypeRevisionMacro(vtkSMPart, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
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
  virtual void SetVTKDataID(vtkClientServerID id);
  vtkClientServerID GetVTKDataID() {return this->VTKDataID;}
  //ETX

  // Description:
  // This method is called on creation.  If the data object is unstructured and 
  // has a maximum number of pieces, then a extract piece filter is inserted
  // before the data object.  This will give parallel pipelines at the
  // expense of initial generation (reading) of the data.
  void InsertExtractPiecesIfNecessary();
  
  // Description:
  // Create the extent translator (sources with no inputs only).
  // Needs to be before "ExtractPieces" because translator propagates.
  void CreateTranslatorIfNecessary();

  // Description:
  // Moving away from direct access to VTK data objects.
  //BTX
  vtkPVDataInformation* GetDataInformation();
  vtkGetObjectMacro(ClassNameInformation, vtkPVClassNameInformation);
  //ETX
  
  // Description:
  // This method collects data information from all processes.
  // This needs to be called before this parts information
  // is valid.
  void GatherDataInformation();

  // Description:
  // Called by source EndEvent to schedule another Gather.
  void InvalidateDataInformation();

  // Description:
  // The name is just a string that will be used in the extract part UI.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

protected:
  vtkSMPart();
  ~vtkSMPart();

  // A part needs a name to show in the extract part filter.
  // We are also going to allow expresion matching.
  char *Name;

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;
  
  vtkPVClassNameInformation *ClassNameInformation;
  
  vtkClientServerID VTKDataID;
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  //void CreateParallelTclObjects(vtkPVApplication *pvApp);

  vtkSMPart(const vtkSMPart&); // Not implemented
  void operator=(const vtkSMPart&); // Not implemented
};

#endif
