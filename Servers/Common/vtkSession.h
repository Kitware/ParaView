/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSession
// .SECTION Description
// vtkSession defines a session i.e. a conversation, if you will. It can be
// between different processes or same process. What types of conversations are
// possible i.e what protocols are supported, is defined by the subclasses.

#ifndef __vtkSession_h
#define __vtkSession_h

#include "vtkObject.h"

class VTK_EXPORT vtkSession : public vtkObject
{
public:
  vtkTypeMacro(vtkSession, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true is this session is active/alive/valid.
  virtual bool GetIsAlive() = 0;

//BTX
protected:
  vtkSession();
  ~vtkSession();

private:
  vtkSession(const vtkSession&); // Not implemented
  void operator=(const vtkSession&); // Not implemented
//ETX
};

#endif
