/*=========================================================================

  Program:   ParaView
  Module:    vtkPVData.h
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

#ifndef __vtkPVData_h
#define __vtkPVData_h

#include "vtkPVActorComposite.h"
#include "vtkDataSet.h"

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
  // Get the number of consumers
  vtkGetMacro(NumberOfPVConsumers, int);
  
  // Description:
  // Add, remove, get, or check a consumer.
  void AddPVConsumer(vtkPVSource *c);
  void RemovePVConsumer(vtkPVSource *c);
  vtkPVSource *GetPVConsumer(int i);
  int IsPVConsumer(vtkPVSource *c);
  
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
  vtkPVSource **PVConsumers;
  int NumberOfPVConsumers;
};

#endif
