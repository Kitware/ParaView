/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPart.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkClientServerStream.h"

class vtkDataSet;
class vtkPVApplication;
class vtkPVClassNameInformation;
class vtkPVDataInformation;
class vtkPVPartDisplay;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;

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

  // Description:
  // The tcl name of the vtk data object.  This should be the primary
  // method of manipulating the data since it exists on all processes.
  // This is for setting up the links between VTK objects and PV
  // object.  This call also sets the input to the mapper.
  // SetVTKDataTclName should be called after the application has been
  // set, but befor PVData is used as input a filter or output of a
  // source.  We could change the object so that it creates its own
  // data (during initia but then we would have to tell it what type
  // of data to create.
//  virtual void SetVTKDataTclName(const char* tclName);
//  vtkGetStringMacro(VTKDataTclName);  

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

  // Description:
  // This method is called on creation.  If the data object is unstructured and 
  // has a maximum number of pieces, then a extract piece filter is inserted
  // before the data object.  This will give parallel pipelines at the
  // expense of initial generation (reading) of the data.
  void InsertExtractPiecesIfNecessary();
  
  //===================
          
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
        
  // Description:
  // Get the tcl name of the vtkPVGeometryFilter.
//  vtkGetStringMacro(GeometryTclName);
  vtkClientServerID GetGeometryID() {return this->GeometryID;}
  

  // Description:
  // Moving away from direct access to VTK data objects.
  vtkPVDataInformation* GetDataInformation();
  vtkGetObjectMacro(ClassNameInformation, vtkPVClassNameInformation);
  
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

protected:
  vtkPVPart();
  ~vtkPVPart();

  vtkPVPartDisplay* PartDisplay;
  
  // A part needs a name to show in the extract part filter.
  // We are also going to allow expresion matching.
  char *Name;

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;
  
  vtkPVClassNameInformation *ClassNameInformation;
  
  char *VTKDataTclName;
  vtkClientServerID GeometryID;
  vtkClientServerID VTKDataID;
  
    
//  char *GeometryTclName;
//  vtkSetStringMacro(GeometryTclName);

  // Here to create unique names.
  int InstanceCount;

  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);

  // If the data changes, we need to change to.
  vtkTimeStamp UpdateTime;

  vtkPVPart(const vtkPVPart&); // Not implemented
  void operator=(const vtkPVPart&); // Not implemented
};

#endif
