/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPhastaReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPPhastaReader - parallel Phasta meta-file reader
// vtkPPhastaReader reads XML based Phasta meta-files and the underlying
// Phasta files. The meta-file has the following form:
// @verbatim
// <?xml version="1.0" ?>
//
// <PhastaMetaFile number_of_pieces="2">
//   <GeometryFileNamePattern pattern="geombc.dat.%d" 
//                            has_piece_entry="1"
//                            has_time_entry="0"/>
//   <FieldFileNamePattern pattern="restart.%d.%d"
//                         has_piece_entry="1"
//                         has_time_entry="1"/>
//
//   <TimeSteps number_of_steps="3" 
//              auto_generate_indices="1"
//              start_index="0"
//              increment_index_by="20">
//     <TimeStep index="0" geometry_index="" field_index="0" value="0.1"/>
//     <TimeStep index="1" geometry_index="" field_index="20" value="0.2"/>
//   </TimeSteps>
//</PhastaMetaFile>
// @endverbatim
// The GeometryFileNamePattern and FieldFileNamePattern elements have
// three attributes:
// @verbatim
// 1. pattern: This is the pattern used to get the Phasta filenames.
//   The %d placeholders will be replaced by appropriate 
//   indices. The first index is time (if specified), the
//   second one is piece.
// 2. has_piece_entry (0 or 1): Specifies whether the pattern has a
//   piece placeholder. The piece placeholder is replaced by the 
//   update piece number.
// 3. has_time_entry (0 or 1): Specified whether the pattern has a
//   time placeholder. The time placeholder is replaced by an index
//   specified in the TimeSteps element
//
// The TimeSteps element contains TimeStep sub-elements. Each TimeStep
// element specifies an index (index attribute), an index used in the
// geometry filename pattern (geometry_index), an index used in the
// field filename pattern (field_index) and a time value (float).
// In the example above, there are two timesteps. The first one is
// stored in files named geombc.dat.(0,1), restart.(0,20).(0,1).
// The time placeholders are substituted with the the geometry_index
// and field_index attribute values.
// 
// Normally there is one TimeStep element per timestep. However, it
// is possible to ask the reader to automatically generate timestep
// entries. This is done with setting the (optional) auto_generate_indices
// to 1. In this case, the reader will generate number_of_steps entries.
// The geometry_index and field_index of these entries will start at
// start_index and will be incremented by increment_index_by. 
// Note that it is possible to use a combination of both approaches.
// Any values specified with the TimeStep elements will overwrite anything
// automatically computed. A common use of this is to let the reader
// compute all indices for field files and overwrite the geometry indices
// with TimeStep elements.
// .SECTION See Also
// vtkPhastaReader

#ifndef __vtkPPhastaReader_h
#define __vtkPPhastaReader_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkPVXMLParser;
class vtkPhastaReader;

//BTX
struct vtkPPhastaReaderInternal;
//ETX

class VTK_EXPORT vtkPPhastaReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPPhastaReader* New();
  vtkTypeRevisionMacro(vtkPPhastaReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set and get the Phasta meta file name
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Set the step number for the geometry.
  vtkSetClampMacro(TimeStepIndex, int, 0, VTK_LARGE_INTEGER);
  vtkGetMacro(TimeStepIndex, int);

  // Description:
  // The min and max values of timesteps.
  vtkGetVector2Macro(TimeStepRange, int);

protected:
  vtkPPhastaReader();
  ~vtkPPhastaReader();
  
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  char* FileName;

  int TimeStepIndex;

  // Descriptions:
  // Store the range of time steps
  int TimeStepRange[2];

  vtkPhastaReader* Reader;
  vtkPVXMLParser* Parser;

private:
  vtkPPhastaReaderInternal* Internal;
  
  vtkPPhastaReader(const vtkPPhastaReader&);  // Not implemented.
  void operator=(const vtkPPhastaReader&);  // Not implemented.
};

#endif

