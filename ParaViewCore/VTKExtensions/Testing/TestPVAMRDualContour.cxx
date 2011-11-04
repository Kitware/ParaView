/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRDualContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <iostream>

#include "vtkPVAMRDualContour.h"
#include "vtkSmartPointer.h"
#include "vtkSpyPlotReader.h"
#include "vtkTestUtilities.h"
#include "vtkDummyController.h"
#include "vtkMultiProcessController.h"

typedef vtkSmartPointer<vtkDummyController> vtkDummyControllerRefPtr;
typedef vtkSmartPointer<vtkSpyPlotReader> vtkSpyPlotReaderRefPtr;
typedef vtkSmartPointer<vtkPVAMRDualContour> vtkPVAMRDualContourRefPtr;

int main( int argc, char* argv[] )
{
  vtkDummyControllerRefPtr controller (vtkDummyControllerRefPtr::New());
  vtkMultiProcessController::SetGlobalController(controller);

  int rc            = 0;
  const char *fname ="/media/shared/Data/ParaViewData/Data/SPCTH/spcth.0";

  std::cout << "Opening file...";
  std::cout.flush();
  vtkSpyPlotReaderRefPtr reader = vtkSpyPlotReaderRefPtr::New();
  std::cout << "Set file name...";
  std::cout.flush();
  reader->SetFileName( fname );
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "Set global controller...";
  reader->SetGlobalController(controller);
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "MergeXYZComponents...";
  reader->MergeXYZComponentsOn();
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "DownConvertVolumeFractionOn...";
  reader->DownConvertVolumeFractionOn();
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "DistributeFilesOn...";
  reader->DistributeFilesOn();
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "SetCellArrayStatus...";
  reader->SetCellArrayStatus("Material volume fraction -3", 1);
  std::cout << "[DONE]\n";
  std::cout.flush();

  std::cout << "Reader.Update()...";
  std::cout.flush();
  reader->Update();
  std::cout << "[DONE]\n";
  std::cout.flush();

  vtkPVAMRDualContourRefPtr contour = vtkPVAMRDualContourRefPtr::New();
  contour->SetInput( reader->GetOutputDataObject(0) );
  contour->SetVolumeFractionSurfaceValue(0.1);
  contour->SetEnableMergePoints(1);
  contour->SetEnableDegenerateCells(1);
  contour->SetEnableMultiProcessCommunication(1);
  contour->AddInputCellArrayToProcess("Material volume fraction - 3");
  contour->Update();

  return( rc );
}
