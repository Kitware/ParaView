/*=========================================================================

  Program:   ParaView
  Module:    vtkEnsembleReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 .NAME vtkEnsembleReader - Meta-reader for an ensemble of simulations

 .SECTION Description:
 The meta-file that describes the simulation ensemble has the following format:
<pre>
  parameterName_1, parameterName_2, ..., parameterName_p,
  v_11,            v_12,            ..., v_1p,            fileName_1
  v_21,            v_22,            ..., v_2p,            fileName_2
  ...
  v_n1,            v_n2,            ..., v_np,            fileName_n

  where parameterName_j is the name of a simulation parameter, j = 1..p
        p is the number of parameters
        v_ij is the value of parameter_j for simulation i, i = 1..n
        n is the number of data files (simulations)
        fileName_i is the data file i, specified as either an absolute file or
        a file relative to the meta-file location.
</pre>
*/


#ifndef __vtkEnsembleReader_h
#define __vtkEnsembleReader_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkMetaReader.h"
#include "vtkSmartPointer.h" // for internal API
#include <string> // std string

class vtkTable;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkEnsembleReader : public vtkMetaReader
{
public:
  static vtkEnsembleReader *New();
  vtkTypeMacro(vtkEnsembleReader,vtkMetaReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of simulations (files) in the ensemble
  vtkIdType GetNumberOfFiles() const;

  // Description:
  // Get the number of simulation parameters specified for each simulation file.
  vtkIdType GetNumberOfParameters () const;

  // Description:
  // Initialize the current reader on REQUEST_INFORMATION, forward all
  // other requests to it.
  virtual int ProcessRequest(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

protected:
  vtkEnsembleReader();
  ~vtkEnsembleReader();

  void UpdateReader();
  vtkSmartPointer<vtkTable> ReadMetaFile(const char* metaFileName);
  std::string GetReaderFileName(const char* metaFileName,
                                vtkTable* table, vtkIdType i);
  void AddCurrentTableRow(vtkInformationVector* outputVector);


protected:
  // Table containing parameter names, values and file names.
  vtkSmartPointer<vtkTable> Table;

private:
  vtkEnsembleReader(const vtkEnsembleReader&); // Not Implemented
  void operator=(const vtkEnsembleReader&); // Not Implemented
};



#endif
