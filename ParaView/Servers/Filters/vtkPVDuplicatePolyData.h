/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDuplicatePolyData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPVDuplicatePolyData - For distributed tiled displays.
//
// .SECTION Description
// This filter collects polydata from all nodes 
// and duplicates it on every display node.
// Converts data parallel so every display node has a complete 
// copy of the data.  The filter is used at the end of a pipeline 
// for driving a tiled display.  This filter does not duplicate if the
// data will be over a specified threshold memory size.
//
// .SECTION Note
// This filter uses binary trees to try to involve all processes.
// I do not know if this is any better than just direct sends.
// It may be worse.

#ifndef __vtkPVDuplicatePolyData_h
#define __vtkPVDuplicatePolyData_h

#include "vtkPolyDataToPolyDataFilter.h"
class vtkSocketController;
class vtkMultiProcessController;
class vtkTiledDisplaySchedule;

class VTK_EXPORT vtkPVDuplicatePolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkPVDuplicatePolyData *New();
  vtkTypeRevisionMacro(vtkPVDuplicatePolyData, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  void InitializeSchedule(int numTiles);

  // Description:
  // This duplicate filter works in client server mode when this
  // controller is set.  We have a client flag to diferentiate the
  // client and server because the socket controller is odd:
  // Both processes think their id is 0.
  vtkSocketController *GetSocketController() {return this->SocketController;}
  void SetSocketController (vtkSocketController *controller);
  vtkSetMacro(ClientFlag,int);
  vtkGetMacro(ClientFlag,int);

  // Description:
  // Turn the filter on or off.  ParaView disable this filter when it will
  // use compositing instead of local rendering.  This flag is off by default.
  vtkSetMacro(PassThrough,int);
  vtkGetMacro(PassThrough,int);
  vtkBooleanMacro(PassThrough,int);

  // Description:
  // This flag should be set on all processes when MPI root
  // is used as client.
  vtkSetMacro(ZeroEmpty,int);
  vtkGetMacro(ZeroEmpty,int);
  vtkBooleanMacro(ZeroEmpty,int);

protected:
  vtkPVDuplicatePolyData();
  ~vtkPVDuplicatePolyData();

  // Data generation method
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ClientExecute(vtkMultiProcessController* controller);
  void ExecuteInformation();

  vtkMultiProcessController *Controller;
  vtkTiledDisplaySchedule* Schedule;

  // For client server mode.
  vtkSocketController *SocketController;
  int ClientFlag;

  int PassThrough;
  int ZeroEmpty;

private:
  vtkPVDuplicatePolyData(const vtkPVDuplicatePolyData&); // Not implemented
  void operator=(const vtkPVDuplicatePolyData&); // Not implemented
};

#endif

