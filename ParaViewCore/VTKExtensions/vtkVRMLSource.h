/*=========================================================================

  Program:   ParaView
  Module:    vtkVRMLSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVRMLSource - Converts importer to a source.
// .SECTION Description
// Since paraview can only use vtkSources, I am wrapping the VRML importer
// as a source.  I will loose lights, texture maps and colors,

#ifndef __vtkVRMLSource_h
#define __vtkVRMLSource_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkMultiBlockDataSet;
class vtkVRMLImporter;

class VTK_EXPORT vtkVRMLSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkVRMLSource,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkVRMLSource *New();

  // Description:
  // VRML file name.  Set
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description: 
  // Descided whether to generate color arrays or not.
  vtkSetMacro(Color,int);
  vtkGetMacro(Color,int);
  vtkBooleanMacro(Color,int);

  // Description:
  // This method allows all parts to be put into a single output.
  // By default this flag is on.
  vtkSetMacro(Append,int);
  vtkGetMacro(Append,int);
  vtkBooleanMacro(Append,int);

  static int CanReadFile(const char *filename);

protected:
  vtkVRMLSource();
  ~vtkVRMLSource();

  int RequestData(vtkInformation*, 
                  vtkInformationVector**, 
                  vtkInformationVector*);

  void InitializeImporter();
  void CopyImporterToOutputs(vtkMultiBlockDataSet*);

  char* FileName;
  vtkVRMLImporter *Importer;
  int Color;
  int Append;

private:
  vtkVRMLSource(const vtkVRMLSource&);  // Not implemented.
  void operator=(const vtkVRMLSource&);  // Not implemented.
};

#endif

