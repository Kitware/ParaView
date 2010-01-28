/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMantaManager.h"
#include "vtkObjectFactory.h"
#include "vtkManta.h"

#include <Interface/Scene.h>
#include <Interface/Object.h>
#include <Interface/Context.h>
#include <Engine/Control/RTRT.h>
#include <Engine/Factory/Create.h>
#include <Engine/Factory/Factory.h>
#include <Engine/Display/NullDisplay.h>
#include <Engine/Display/SyncDisplay.h>
#include <Engine/Control/RTRT.h>

vtkCxxRevisionMacro(vtkMantaManager, "1.1");
vtkStandardNewMacro(vtkMantaManager);

//----------------------------------------------------------------------------
vtkMantaManager::vtkMantaManager()
{
  cerr << "CREATE MANTA MANAGER " << this << endl;

  this->MantaEngine = Manta::createManta();
  this->MantaFactory = new Manta::Factory( this->MantaEngine );
}

//----------------------------------------------------------------------------
vtkMantaManager::~vtkMantaManager()
{
  cerr << "DESTROY MANTA MANAGER " << this << endl;

  this->MantaEngine->finish();
  this->MantaEngine->blockUntilFinished();

  delete this->MantaFactory;
  delete this->MantaEngine;
}

//----------------------------------------------------------------------------
void vtkMantaManager::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
