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
#include "vtkMPIMoveData.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int TestMPI(int, char* [])
{
  vtkConeSource* cone = vtkConeSource::New();
  //   cone->Update(); //For GetCenter

  vtkPVConnectivityFilter* connect = vtkPVConnectivityFilter::New();
  connect->SetInputConnection(cone->GetOutputPort());

  vtkMPIMoveData* move = vtkMPIMoveData::New();
  move->SetInputConnection(connect->GetOutputPort());
  move->SetMoveModeToPassThrough();
  move->Update();

  cone->Delete();
  connect->Delete();
  move->Delete();

  return 0;
}
