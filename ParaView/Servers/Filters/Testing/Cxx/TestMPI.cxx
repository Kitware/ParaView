/*=========================================================================

  Program:   ParaView
  Module:    TestMPI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConeSource.h"
#include "vtkPickFilter.h"
#include "vtkMPIDuplicatePolyData.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkMPIDuplicateUnstructuredGrid.h"
#include "vtkMPIMoveData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int main(int , char* [])
{
  vtkConeSource *cone = vtkConeSource::New();
  cone->Update(); //For GetCenter
  
  vtkPickFilter *pick = vtkPickFilter::New();
  
  //law int fixme;  // why is this a problem.
  //pick->SetInput( cone->GetOutput() );
  pick->SetWorldPoint ( cone->GetOutput()->GetCenter() );
  pick->GetWorldPoint ();
  pick->SetPickCell ( 1 );
  pick->Update();

  vtkMPIDuplicatePolyData *duplicate = vtkMPIDuplicatePolyData::New();
  duplicate->SetInput( cone->GetOutput() );
  duplicate->PassThroughOn ();

  vtkPVConnectivityFilter *connect = vtkPVConnectivityFilter::New();
  connect->SetInput( duplicate->GetOutput() );
  
  vtkMPIDuplicateUnstructuredGrid *dupUns = vtkMPIDuplicateUnstructuredGrid::New();
  dupUns->SetInput( connect->GetOutput() );
  dupUns->PassThroughOn ();
  
  vtkMPIMoveData *move = vtkMPIMoveData::New();
  move->SetInput( dupUns->GetOutput() );
  move->SetMoveModeToPassThrough ();
  move->Update();

  cone->Delete();
  pick->Delete();
  duplicate->Delete();
  connect->Delete();
  dupUns->Delete();
  move->Delete();

  return 0;
}
