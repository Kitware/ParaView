/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTimerInformation
 * @brief   Holds timer log for all processes.
 *
 * I am using this information object to gather timer logs from all processes.
*/

#ifndef vtkPVTimerInformation_h
#define vtkPVTimerInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVTimerInformation : public vtkPVInformation
{
public:
  static vtkPVTimerInformation* New();
  vtkTypeMacro(vtkPVTimerInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the threshold to use to gather the timer log information. This must
   * be set before calling GatherInformation().
   */
  vtkSetMacro(LogThreshold, double);
  vtkGetMacro(LogThreshold, double);
  //@}

  //@{
  /**
   * Access to the logs.
   */
  int GetNumberOfLogs();
  char* GetLog(int proc);
  //@}

  //@{
  /**
   * Transfer information about a single object into
   * this object.
   */
  virtual void CopyFromObject(vtkObject* data) VTK_OVERRIDE;
  virtual void CopyFromMessage(unsigned char* msg);
  //@}

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation* info) VTK_OVERRIDE;

  //@{
  /**
   * Serialize objects to/from a stream object.
   */
  virtual void CopyToStream(vtkClientServerStream*) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream* css) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself.
   */
  virtual void CopyParametersToStream(vtkMultiProcessStream&) VTK_OVERRIDE;
  virtual void CopyParametersFromStream(vtkMultiProcessStream&) VTK_OVERRIDE;

protected:
  vtkPVTimerInformation();
  ~vtkPVTimerInformation();
  //@}

  void Reallocate(int num);
  void InsertLog(int id, const char* log);

  double LogThreshold;
  int NumberOfLogs;
  char** Logs;

  vtkPVTimerInformation(const vtkPVTimerInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVTimerInformation&) VTK_DELETE_FUNCTION;
};

#endif
