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
#include "vtkPVConnectivityFilter.h"
#include "vtkMPIMoveData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int main(int , char* [])
{
  vtkConeSource *cone = vtkConeSource::New();
  cone->Update(); //For GetCenter
  
  vtkPVConnectivityFilter *connect = vtkPVConnectivityFilter::New();
  connect->SetInput( cone->GetOutput() );
  
  vtkMPIMoveData *move = vtkMPIMoveData::New();
  move->SetInput( connect->GetOutput() );
  move->SetMoveModeToPassThrough ();
  move->Update();

  cone->Delete();
  connect->Delete();
  move->Delete();

  return 0;
}
