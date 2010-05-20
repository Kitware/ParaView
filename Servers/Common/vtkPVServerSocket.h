/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerSocket.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerSocket
// .SECTION Description
// This class is created only to maintain an additional iVar with the server
// socket. This ivar indicates the type of the socket 
// i.e. render/data/render and data.

#ifndef __vtkPVServerSocket_h
#define __vtkPVServerSocket_h

#include "vtkServerSocket.h"

class VTK_EXPORT vtkPVServerSocket : public vtkServerSocket
{
public:
  static vtkPVServerSocket* New();
  vtkTypeMacro(vtkPVServerSocket, vtkServerSocket);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the type of the server socket.
  vtkGetMacro(Type, int);
  vtkSetMacro(Type, int);
protected:
  vtkPVServerSocket();
  ~vtkPVServerSocket();

  int Type;

private:
  vtkPVServerSocket(const vtkPVServerSocket&); // Not implemented.
  void operator=(const vtkPVServerSocket&); // Not implemented.
};


#endif

