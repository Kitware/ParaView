/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkM2NDuplicate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkM2NDuplicate - For distributed tiled displays.
// .DESCRIPTION
// 


#ifndef __vtkM2NDuplicate_h
#define __vtkM2NDuplicate_h

#include "vtkMPIDuplicatePolyData.h"
class vtkMPIMToNSocketConnection;


class VTK_EXPORT vtkM2NDuplicate : public vtkMPIDuplicatePolyData
{
public:
  static vtkM2NDuplicate *New();
  vtkTypeRevisionMacro(vtkM2NDuplicate, vtkMPIDuplicatePolyData);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection*);
  vtkSetMacro(ServerMode, int);
  vtkGetMacro(ServerMode, int);
  vtkSetMacro(RenderServerMode, int);
  vtkGetMacro(RenderServerMode, int);
  vtkSetMacro(ClientMode, int);
  vtkGetMacro(ClientMode, int);
  
protected:
  vtkM2NDuplicate();
  ~vtkM2NDuplicate();

  // Data generation method
  virtual void ComputeInputUpdateExtents(vtkDataObject *output);
  virtual void Execute();
  virtual void ExecuteInformation();

private:
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;
  int ServerMode;
  int RenderServerMode;
  int ClientMode;
  vtkM2NDuplicate(const vtkM2NDuplicate&); // Not implemented
  void operator=(const vtkM2NDuplicate&); // Not implemented
};

#endif

