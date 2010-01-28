/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMantaManager - persistant access to Manta engine
// .SECTION Description
// vtkMantaManager is a reference counted wrapper around the manta engine.
// Because it is reference counted, it outlives all vtkManta classes that 
// reference it. That means that they can safely use it to manage their
// own Manta side resources and that the engine itself will be destructed
// when the wrapper is.

#ifndef __vtkMantaManager_h
#define __vtkMantaManager_h

#include "vtkObject.h"
#include "vtkMantaConfigure.h"

//BTX
namespace Manta {
class MantaInterface;
class Factory;
};
//ETX

class VTK_vtkManta_EXPORT vtkMantaManager : public vtkObject
{
public:
  static vtkMantaManager *New();
  vtkTypeRevisionMacro(vtkMantaManager,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  Manta::MantaInterface* GetMantaEngine()    
  { 
  return this->MantaEngine;
  }
  Manta::Factory* GetMantaFactory()
  { 
    return this->MantaFactory;
  }
//ETX    

 protected:
  vtkMantaManager();
  ~vtkMantaManager();
  
 private:
  vtkMantaManager(const vtkMantaManager&);  // Not implemented.
  void operator=(const vtkMantaManager&);  // Not implemented.
    
  //BTX
  Manta::MantaInterface * MantaEngine;
  Manta::Factory * MantaFactory;
  //ETX
};

#endif
