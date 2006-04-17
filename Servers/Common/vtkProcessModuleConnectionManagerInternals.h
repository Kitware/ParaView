/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleConnectionManagerInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClientSocket.h"
#include "vtkSmartPointer.h"
#include "vtkProcessModuleConnection.h"
#include "vtkPVServerSocket.h"

#include <vtkstd/map>
#include <vtkstd/deque>

class vtkProcessModuleConnectionManagerInternals
{
public:
  typedef vtkstd::map<vtkSocket*, 
          vtkSmartPointer<vtkProcessModuleConnection> > MapOfSocketToConnection;

  typedef vtkstd::map<vtkIdType, 
          vtkSmartPointer<vtkProcessModuleConnection> > MapOfIDToConnection;

  typedef vtkstd::map<int, vtkSmartPointer<vtkPVServerSocket> > MapOfIntToPVServerSocket;

  typedef vtkstd::deque< vtkSmartPointer<vtkClientSocket> > QueueOfConnections;
  
  MapOfSocketToConnection SocketToConnectionMap;
  MapOfIDToConnection IDToConnectionMap;
  MapOfIntToPVServerSocket IntToServerSocketMap;

  QueueOfConnections DataServerConnections;
  QueueOfConnections RenderServerConnections;
};

