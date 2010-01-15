/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile FortranAdaptorAPI.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef FortranAdaptorAPI_h
#define FortranAdaptorAPI_h

class vtkCPDataDescription;
class vtkDataSet;

// function to return the singleton/static vtkCPDataDescription object
// that contains the grid and fields stuff
vtkCPDataDescription* GetCoProcessorData();
void ClearFieldDataFromGrid(vtkDataSet* grid);
bool ConvertFortranStringToCString(char* fortranString, int fortranStringLength,
                                   char* cString, int cStringMaxLength);

// for now assume that the coprocessor is run through a python script
extern "C" void coprocessorinitialize_(char* pythonFileName, int* pythonFileNameLength);
extern "C" void coprocessorfinalize_();
// this is the function that determines whether or not there
// is anything to coprocess this time step
extern "C" void requestdatadescription_(int* timeStep, double* time, 
                                        int* coprocessThisTimeStep);
// this function sets needgrid to 1 if it does not have a copy of the grid
// it sets needgrid to 0 if it does have a copy of the grid but does not
// check if the grid is modified or needs to be updated
extern "C" void needtocreategrid_(int* needGrid);
// do the actual coprocessing.  it is assumed that the vtkCPDataDescription
// has been filled in elsewhere.
extern "C" void coprocess_();

#endif
