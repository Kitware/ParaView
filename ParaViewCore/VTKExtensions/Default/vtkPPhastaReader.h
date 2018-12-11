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
/**
 * @class   vtkPPhastaReader
 * @brief   parallel Phasta meta-file reader
 * vtkPPhastaReader reads XML based Phasta meta-files and the underlying
 * Phasta files. The meta-file has the following form:
 * @verbatim
 * <?xml version="1.0" ?>
 *
 * <PhastaMetaFile number_of_pieces="2">
 *   <GeometryFileNamePattern pattern="geombc.dat.%d"
 *                            has_piece_entry="1"
 *                            has_time_entry="0"/>
 *   <FieldFileNamePattern pattern="restart.%d.%d"
 *                         has_piece_entry="1"
 *                         has_time_entry="1"/>
 *
 *   <TimeSteps number_of_steps="2"
 *              auto_generate_indices="1"
 *              start_index="0"
 *              increment_index_by="20"
 *              start_value="0."
 *              increment_value_by="20.">
 *     <TimeStep index="0" geometry_index="" field_index="0" value="0.0"/>
 *     <TimeStep index="1" geometry_index="" field_index="20" value="20.0"/>
 *   </TimeSteps>
 *   <Fields number_of_fields="2">
 *     <Field paraview_field_tag="velocity"
 *            phasta_field_tag="solution"
 *            start_index_in_phasta_array="1"
 *            number_of_componenets="3"
 *            datadependency="0"
 *            data_type="double"/>
 *     <Field paraview_field_tag="average speed"
 *            phasta_field_tag="ybar"
 *            start_index_in_phasta_array="4"
 *            number_of_componenets="1"/>
 *   </Fields>
 *</PhastaMetaFile>
 * @endverbatim
 * The GeometryFileNamePattern and FieldFileNamePattern elements have
 * three attributes:
 * \li pattern: This is the pattern used to get the Phasta filenames.
 *   The %d placeholders will be replaced by appropriate
 *   indices. The first index is time (if specified), the
 *   second one is piece.
 * \li has_piece_entry (0 or 1): Specifies whether the pattern has a
 *   piece placeholder. The piece placeholder is replaced by the
 *   update piece number.
 * \li has_time_entry (0 or 1): Specified whether the pattern has a
 *   time placeholder. The time placeholder is replaced by an index
 *   specified in the TimeSteps element
 *
 * The TimeSteps element contains TimeStep sub-elements. Each TimeStep
 * element specifies an index (index attribute), an index used in the
 * geometry filename pattern (geometry_index), an index used in the
 * field filename pattern (field_index) and a time value (float).
 * In the example above, there are two timesteps. The first one is
 * stored in files named geombc.dat.(0,1), restart.(0,20).(0,1).
 * The time placeholders are substituted with the the geometry_index
 * and field_index attribute values.
 *
 * Normally there is one TimeStep element per timestep. However, it
 * is possible to ask the reader to automatically generate timestep
 * entries. This is done with setting the (optional) auto_generate_indices
 * to 1. In this case, the reader will generate number_of_steps entries.
 * The geometry_index and field_index of these entries will start at
 * start_index and will be incremented by increment_index_by.
 * The time value of these entries will start at start_value and
 * will be incremented by increment_value_by.
 * Note that it is possible to use a combination of both approaches.
 * Any values specified with the TimeStep elements will overwrite anything
 * automatically computed. A common use of this is to let the reader
 * compute all indices for field files and overwrite the geometry indices
 * with TimeStep elements.
 *
 * The Fields element contain number_of_fields Field sub-element.
 * Each Field element specifies tag attribute to be used in paraview,
 * tag under which the field is stored in phasta files, start index of
 * the array in phasta files, number of components of the field, data
 * dependency, i.e., either 0 for nodal or 1 for elemental and
 * data type, i.e., "double" (as of now supports only 1, 3 & 9 for number
 * of components, i.e., scalars, vectors & tensors, and "double" for
 * type of data).
 * In the example above, there are two fields to be visualized
 * one is velocity field stored under tag solution from index 1 to 3
 * in phasta files which is a vector field defined on nodes with
 * double values, and the other field is average speed scalar field
 * stored under tag ybar at index 4 in phasta files
 * If any Field element is specified then default attribute values are :
 * (phasta_field_tag is mandatory)
 * paraview_field_tag = Field \<number\>
 * start_index_in_phasta_array = 0
 * number_of_components = 1
 * data_dependency = 0
 * data_type = double
 * If no Field(s) is/are specified then the default is following 3 fields :
 * pressure (index 0 under solution),
 * velocity (index 1-3 under solution)
 * temperature (index 4 under soltuion)
 *
 * @sa
 * vtkPhastaReader
*/

#ifndef vtkPPhastaReader_h
#define vtkPPhastaReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkPVXMLParser;
class vtkPhastaReader;

struct vtkPPhastaReaderInternal;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkPPhastaReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPPhastaReader* New();
  vtkTypeMacro(vtkPPhastaReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set and get the Phasta meta file name
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set the step number for the geometry.
   */
  vtkSetClampMacro(TimeStepIndex, int, 0, VTK_INT_MAX);
  vtkGetMacro(TimeStepIndex, int);
  //@}

  //@{
  /**
   * The min and max values of timesteps.
   */
  vtkGetVector2Macro(TimeStepRange, int);
  //@}

  static int CanReadFile(const char* filename);

protected:
  vtkPPhastaReader();
  ~vtkPPhastaReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  char* FileName;

  int TimeStepIndex;

  /**
   * Store the range of time steps
   */
  int TimeStepRange[2];

  vtkPhastaReader* Reader;
  vtkPVXMLParser* Parser;

  int ActualTimeStep;

private:
  vtkPPhastaReaderInternal* Internal;

  vtkPPhastaReader(const vtkPPhastaReader&) = delete;
  void operator=(const vtkPPhastaReader&) = delete;
};

#endif
