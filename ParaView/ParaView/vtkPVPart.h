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


#include "vtkKWObject.h"

#include "vtkClientServerID.h" // Needed For Set Get VTKDataID

class vtkDataSet;
class vtkPVApplication;
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkPVPartDisplay;
class vtkPVDisplay;
class vtkPolyDataMapper;
class vtkCollection;

class VTK_EXPORT vtkPVPart : public vtkKWObject
{
public:
  static vtkPVPart* New();
  vtkTypeRevisionMacro(vtkPVPart, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This also creates the parallel vtk objects for the composite.
  // (actor, mapper, ...)
  void SetPVApplication(vtkPVApplication *pvApp);
  void SetApplication(vtkKWApplication *)
    {
    vtkErrorMacro("vtkPVPart::SetApplication should not be used. Use SetPVApplcation instead.");
    }

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


  //===================
          
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
  //BTX
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
  // Called by source EndEvent to schedule another Gather.
  void InvalidateDataInformation();

  // Description:
  // The name is just a string that will be used in the extract part UI.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Temporary access to the display object.
  // Render modules may eleimnate the need for this access.
  vtkPVPartDisplay* GetPartDisplay() { return this->PartDisplay;}
  void SetPartDisplay(vtkPVPartDisplay* pDisp);

  // Description:
  // We are starting to support multiple types of displays (plot).
  // I am keeping the PartDisplay pointer and methods around
  // until we come up with a better API (maybe proxy/properties).
  // The method SetPartDisplay also adds the display to this collection.
  void AddDisplay(vtkPVDisplay* disp);

  // Description:
  // VTKSourceIndex points to the VTKSourceID in this
  // part's PVSource. The tcl name of the VTK source that produced
  // the data in this part can be obtained with
  // source->GetVTKSourceID(part->GetVTKSourceIndex())
  // This is used during batch file generation.
  vtkGetMacro(VTKSourceIndex, int);
  vtkSetMacro(VTKSourceIndex, int);

  // Description:
  // VTKOutputIndex together with  VTKSourceID is used
  // to obtain the source of the data object in this part.
  // For example, the output in this data object is obtained
  // with "%s GetOutput %d",
  // source->GetVTKSourceID(part->GetVTKSourceIndex()),
  // part->GetVTKOutputIndex()
  // This is used during batch file generation.
  vtkGetMacro(VTKOutputIndex, int);
  vtkSetMacro(VTKOutputIndex, int);

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

protected:
  vtkPVPart();
  ~vtkPVPart();

  // We are starting to support multiple types of displays (plot).
  // I am keeping the PartDisplay pointer and methods around
  // until we come up with a better API (maybe proxy/properties).
  // The part display is also in the collection.
  vtkCollection* Displays;
  vtkPVPartDisplay* PartDisplay;
  
  // A part needs a name to show in the extract part filter.
  // We are also going to allow expresion matching.
  char *Name;

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;
  
  vtkPVClassNameInformation *ClassNameInformation;
  
  vtkClientServerID VTKDataID;
  
  // Here to create unique names.
  int InstanceCount;

  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);

  // If the data changes, we need to change to.
  vtkTimeStamp UpdateTime;

  int VTKSourceIndex;
  int VTKOutputIndex;

  vtkPVPart(const vtkPVPart&); // Not implemented
  void operator=(const vtkPVPart&); // Not implemented
};

#endif
