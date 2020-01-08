/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDataArraySelectionProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSIDataArraySelectionProperty
 * @brief supports API using vtkDataArraySelection.
 *
 * vtkSIDataArraySelectionProperty can be used to get/set array selection status
 * parameters on a VTK object using vtkDataArraySelection. Readers typically
 * offer API to get information about data array available as well as
 * enable/disable arrays to read using a vtkDataArraySelection instance
 * e.g. `vtkXMLReader::GetPointDataArraySelection`. This si-property can be used
 * on vtkSMStringVectorProperty instances that use such an API on the reader to
 * both get array selection status as well as set them.
 *
 * This offers a convenient replacement for vtkSIArraySelectionProperty which
 * requires a quirky API on the VTK class to get/set similar information and is
 * recommended over vtkSIArraySelectionProperty in new code.
 *
 * To use vtkSIDataArraySelectionProperty, simply add this as the `si_class` on
 * the properties for getting and setting array selection with `command` set to
 * the method name that returns a mutable vtkDataArraySelection instance.
 *
 * e.g.
 * @code{xml}
 *   <SourceProxy name="AMReXParticlesReader" class="vtkAMReXParticlesReader">
 *     ...
 *     <!-- this is the property to get the status -->
 *     <StringVectorProperty name="PointArrayInfo"
 *       command="GetPointDataArraySelection"
 *       number_of_elements_per_command="2"
 *       information_only="1"
 *       si_class="vtkSIDataArraySelectionProperty" />
 *
 *     <!-- this is the property to set he status -->
 *     <StringVectorProperty name="PointArrayStatus"
 *       command="GetPointDataArraySelection"
 *       information_property="PointArrayInfo"
 *       number_of_elements_per_command="2"
 *       element_types="2 0"
 *       repeat_command="1"
 *       si_class="vtkSIDataArraySelectionProperty">
 *       <ArraySelectionDomain name="array_list">
 *         <RequiredProperties>
 *           <Property function="ArrayList" name="PointArrayInfo" />
 *         </RequiredProperties>
 *       </ArraySelectionDomain>
 *       <Documentation>Select the point arrays to read load.</Documentation>
 *     </StringVectorProperty>
 *     ...
 *  </SourceProxy>
 * @endcode
 *
 * vtkSIDataArraySelectionProperty can also be used for filters that use
 * vtkDataArraySelection.
 *
 * @code{xml}
 *   <SourceProxy class="vtkPassSelectedArrays" name="PassArrays">
 *      ...
 *      <InputProperty name="Input" >
 *        <InputArrayDomain name="point_arrays" attribute_type="point" optional="1" />
 *      </InputProperty>
 *      <StringVectorProperty
 *          name="PointDataArraySelection"
 *          command="GetPointDataArraySelection"
 *          number_of_elements_per_command="1"
 *          repeat_command="1"
 *          si_class="vtkSIDataArraySelectionProperty">
 *          <ArrayListDomain name="array_list" input_domain_name="point_arrays">
 *            <RequiredProperties>
 *              <Property name="Input" function="Input" />
 *            </RequiredProperties>
 *          </ArrayListDomain>
 *      </StringVectorProperty>
 *      ...
 *   </SourceProxy>
 * @endcode
 */

#ifndef vtkSIDataArraySelectionProperty_h
#define vtkSIDataArraySelectionProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class vtkDataArraySelection;
class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIDataArraySelectionProperty : public vtkSIProperty
{
public:
  static vtkSIDataArraySelectionProperty* New();
  vtkTypeMacro(vtkSIDataArraySelectionProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIDataArraySelectionProperty();
  ~vtkSIDataArraySelectionProperty();

  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;
  bool Push(vtkSMMessage*, int) override;
  bool Pull(vtkSMMessage*) override;

  vtkDataArraySelection* GetSelection();

private:
  vtkSIDataArraySelectionProperty(const vtkSIDataArraySelectionProperty&) = delete;
  void operator=(const vtkSIDataArraySelectionProperty&) = delete;
  int NumberOfElementsPerCommand;
};

#endif
