/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkM2NCollect.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkM2NCollect - For distributed tiled displays.
// .DESCRIPTION
// 


#ifndef __vtkM2NCollect_h
#define __vtkM2NCollect_h

#include "vtkCollectPolyData.h"
class vtkMPIMToNSocketConnection;


class VTK_EXPORT vtkM2NCollect : public vtkCollectPolyData
{
public:
  static vtkM2NCollect *New();
  vtkTypeRevisionMacro(vtkM2NCollect, vtkCollectPolyData);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection*);
  vtkSetMacro(ServerMode, int);
  vtkGetMacro(ServerMode, int);
  vtkSetMacro(RenderServerMode, int);
  vtkGetMacro(RenderServerMode, int);
  vtkSetMacro(ClientMode, int);
  vtkGetMacro(ClientMode, int);
  
protected:
  vtkM2NCollect();
  ~vtkM2NCollect();

  // Data generation method
  virtual void ComputeInputUpdateExtents(vtkDataObject *output);
  virtual void ExecuteData(vtkDataObject* outData);
  virtual void ExecuteInformation();

  int ExchangeSizes(int size);
  void ExchangeData(int inSize, char* inBuf, int outSize, char* outBuf);


private:
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;
  int ServerMode;
  int RenderServerMode;
  int ClientMode;
  vtkM2NCollect(const vtkM2NCollect&); // Not implemented
  void operator=(const vtkM2NCollect&); // Not implemented
};

#endif

