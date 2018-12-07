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
/**
 * @class   vtkVRMLSource
 * @brief   Converts importer to a source.
 *
 * Since paraview can only use vtkSources, I am wrapping the VRML importer
 * as a source.  I will loose lights, texture maps and colors,
*/

#ifndef vtkVRMLSource_h
#define vtkVRMLSource_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkMultiBlockDataSet;
class vtkVRMLImporter;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkVRMLSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkVRMLSource, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVRMLSource* New();

  //@{
  /**
   * VRML file name.  Set
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Decided whether to generate color arrays or not.
   */
  vtkSetMacro(Color, int);
  vtkGetMacro(Color, int);
  vtkBooleanMacro(Color, int);
  //@}

  //@{
  /**
   * This method allows all parts to be put into a single output.
   * By default this flag is on.
   */
  vtkSetMacro(Append, int);
  vtkGetMacro(Append, int);
  vtkBooleanMacro(Append, int);
  //@}

  static int CanReadFile(const char* filename);

protected:
  vtkVRMLSource();
  ~vtkVRMLSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void InitializeImporter();
  void CopyImporterToOutputs(vtkMultiBlockDataSet*);

  char* FileName;
  vtkVRMLImporter* Importer;
  int Color;
  int Append;

private:
  vtkVRMLSource(const vtkVRMLSource&) = delete;
  void operator=(const vtkVRMLSource&) = delete;
};

#endif
