/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChomboReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkChomboReader -
// .SECTION Description

#ifndef __vtkChomboReader_h
#define __vtkChomboReader_h

#include "vtkSource.h"

#include "vtkAMRBox.h" // Needed for vector of vtkAMRBox

#include <hdf5.h> // Needed for hid_t
#include <vtkstd/vector> // Needed for vector ivar
#include <vtkstd/string> // Needed for vector ivar

class vtkHierarchicalBoxDataSet;

class VTK_EXPORT vtkChomboReader : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkChomboReader,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkChomboReader* New();

  // Description:
  // Get/Set the name of the input file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get the output of this source.
  vtkHierarchicalBoxDataSet *GetOutput();
  vtkHierarchicalBoxDataSet *GetOutput(int idx);

  // Description:
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  vtkGetMacro(NumberOfLevels, int);

  // Description:
  vtkGetMacro(Dimensionality, int);

  // Description:
  vtkGetMacro(RealType, int);

protected:
  vtkChomboReader();
  ~vtkChomboReader();
  
  // Standard pipeline exectution methods.
  void ExecuteInformation();
  void ExecuteData(vtkDataObject* output);
  
  // The input file's name.
  char* FileName;

//BTX
  int GetIntAttribute( hid_t locID, const char* attrName, int& val );
  int GetRealTypeAttribute( hid_t locID, const char* attrName, double& val );
  int GetStringAttribute( hid_t locID, const char* attrName, vtkstd::string& val );

  void CreateBoxDataType();

  int ReadBoxes( hid_t locID , vtkstd::vector<vtkAMRBox>& boxes );

  enum Real_T
  {
    FLOAT,
    DOUBLE
  };

  typedef vtkstd::vector<vtkAMRBox> LevelBoxesType;
  typedef vtkstd::vector<LevelBoxesType> BoxesType;
  BoxesType Boxes;

  typedef vtkstd::vector<hsize_t> LevelOffsetsType;
  typedef vtkstd::vector<LevelOffsetsType> OffsetsType;
  OffsetsType Offsets;

  vtkstd::vector<double> DXs;
  vtkstd::vector<vtkstd::string> ComponentNames;
//ETX

  int RealType;
  int Dimensionality;
  int NumberOfLevels;
  int NumberOfComponents;
  hid_t BoxDataType;

private:
  vtkChomboReader(const vtkChomboReader&);  // Not implemented.
  void operator=(const vtkChomboReader&);  // Not implemented.
};

#endif
