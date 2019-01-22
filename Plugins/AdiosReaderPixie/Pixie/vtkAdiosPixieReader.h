/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdiosPixieReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAdiosPixieReader - File reader for Pixie/Adios format.
// .SECTION Description
//

#ifndef vtkAdiosPixieReader_h
#define vtkAdiosPixieReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVAdiosReaderPixieModule.h" // for export macro

class vtkDataSetAttributes;
class vtkInformationVector;
class vtkInformation;

class VTKPVADIOSREADERPIXIE_EXPORT vtkAdiosPixieReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAdiosPixieReader* New();
  vtkTypeMacro(vtkAdiosPixieReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the name of the input file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set Read method (ADIOS_READ_METHOD_BP=0, ADIOS_READ_METHOD_DATASPACES=3)
  // The default value is 0, to be file base
  void SetReadMethod(int methodEnum);

  // Description:
  // Set the Adios advanced parameters:
  // Parameters are a series of name=value pairs separated by ";".
  // E.g. "max_chunk_size=200; app_id = 1".
  // List of parameters is documented for each method separately.
  // This function can be also used for some global settings:
  // - verbose=<integer> Set the level of verbosity of ADIOS messages:
  //   0=quiet, 1=errors only, 2= warnings, 3=info, 4=debug
  // - quiet Same as verbose=0
  // - logfile=<path> Redirect all ADIOS messages to a file. in ADIOS 1.4.0,
  //   there is no process level separation. Note that third-party libraries
  //   used by ADIOS will still print their messages to stdout/stderr.
  // - abort_on_error ADIOS will abort the application whenever ADIOS prints
  //   an error message. In ADIOS 1.4.0, there are many error messages in the
  //   writing part that still go to stderr and will not abort the code.
  vtkSetStringMacro(Parameters);
  vtkGetStringMacro(Parameters);

  // Description:
  // Modified the reader to request a new read (from stage), so you can
  // move forward in time.
  void NextStep();

  // Description:
  // Close stream and reopen it to start again at the first step
  void Reset();

  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);

protected:
  vtkAdiosPixieReader();
  ~vtkAdiosPixieReader();

  // Trigger the real data access
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector);

  // The input file's name.
  char* FileName;

  // The Adios Parameters
  char* Parameters;

private:
  vtkAdiosPixieReader(const vtkAdiosPixieReader&) = delete;
  void operator=(const vtkAdiosPixieReader&) = delete;

  class Internals;
  Internals* Internal;
};

#endif
