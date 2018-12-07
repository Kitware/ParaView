/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCatalystSessionCore.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCatalystSessionCore
 *
 * vtkPVCatalystSessionCore is used by vtkSMSession.
 * vtkPVCatalystSessionCore handle catalyst based proxy which don't contains any
 * real data and therefore are not allowed to execute the VTK pipeline.
 * @ingroup LiveInsitu
*/

#ifndef vtkPVCatalystSessionCore_h
#define vtkPVCatalystSessionCore_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkPVSessionCore.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage.
#include "vtkWeakPointer.h"      // needed for vtkMultiProcessController

class vtkPVInformation;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkPVCatalystSessionCore : public vtkPVSessionCore
{
public:
  static vtkPVCatalystSessionCore* New();
  vtkTypeMacro(vtkPVCatalystSessionCore, vtkPVSessionCore);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Gather information about an object referred by the \c globalid.
   * \c location identifies the processes to gather the information from.
   */
  bool GatherInformation(
    vtkTypeUInt32 location, vtkPVInformation* information, vtkTypeUInt32 globalid) override;

  /**
   * Update the data information for a given proxy with the given globalid.
   * This will allow the GatherInformation to work with "fake" VTK pipeline.
   * Return the real corresponding proxy id
   */
  vtkTypeUInt32 RegisterDataInformation(
    vtkTypeUInt32 globalid, unsigned int port, vtkPVInformation* information);

  void UpdateIdMap(vtkTypeUInt32* idMapArray, int size);
  void ResetIdMap();

protected:
  vtkPVCatalystSessionCore();
  ~vtkPVCatalystSessionCore() override;

private:
  vtkPVCatalystSessionCore(const vtkPVCatalystSessionCore&) = delete;
  void operator=(const vtkPVCatalystSessionCore&) = delete;

  class vtkInternal;
  vtkInternal* CatalystInternal;
};

#endif
