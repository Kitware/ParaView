/*=========================================================================

  Program:   ParaView
  Module:    vtkConnectionIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConnectionIterator - iterates over all connections.
// .SECTION Description
// vtkConnectionIterator iterates over all the connections
// in the vtkProcessModuleConnectionManager. It \c MatchConnectionID is set,
// it iterates over only those connection that match the id. By default,
// \c MatchConnectionID is to set to match all connections present in the
// vtkProcessModuleConnectionManager. Special connection Ids such as those
// returned by \c vtkProcessModuleConnectionManager::GetRootServerConnectionID()
// or \c vtkProcessModuleConnectionManager::GetAllServerConnectionsID() can be 
// used to match a particular type of connections.
// .SECTION See Also
// vtkProcessModuleConnectionManager

#ifndef __vtkConnectionIterator_h
#define __vtkConnectionIterator_h

#include "vtkObject.h"

class vtkProcessModuleConnection;
class vtkProcessModuleConnectionManager;
class vtkConnectionIteratorInternals;

class VTK_EXPORT vtkConnectionIterator : public vtkObject
{
public:
  static vtkConnectionIterator* New();
  vtkTypeMacro(vtkConnectionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Go to the beginning.
  void Begin();

  // Description:
  // Is the iterator pointing past the last element.
  int IsAtEnd();

  // Description:
  // Move to the next connection.
  void Next();
  
  // Description:
  // Get/Set the connection ID to match.
  vtkSetMacro(MatchConnectionID, vtkIdType);
  vtkGetMacro(MatchConnectionID, vtkIdType);
  
  // Description:
  // Get/Set the ConnectionManager.
  vtkGetObjectMacro(ConnectionManager, vtkProcessModuleConnectionManager);
  void SetConnectionManager(vtkProcessModuleConnectionManager*);
  
  // Description:
  // Get the connection at the current position.
  vtkProcessModuleConnection* GetCurrentConnection();
  vtkIdType GetCurrentConnectionID();
protected:
  vtkConnectionIterator();
  ~vtkConnectionIterator();

  vtkConnectionIteratorInternals* Internals;
  vtkIdType MatchConnectionID;
  vtkProcessModuleConnectionManager* ConnectionManager;
  int InBegin;
private:
  vtkConnectionIterator(const vtkConnectionIterator&); // Not implemented.
  void operator=(const vtkConnectionIterator&); // Not implemented.
};

#endif

