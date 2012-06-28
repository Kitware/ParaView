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
// .NAME vtkPVTimerInformation - Holds timer log for all processes.
// .SECTION Description
// I am using this information object to gather timer logs from all processes.

#ifndef __vtkPVTimerInformation_h
#define __vtkPVTimerInformation_h


#include "vtkPVInformation.h"


class VTK_EXPORT vtkPVTimerInformation : public vtkPVInformation
{
public:
  static vtkPVTimerInformation* New();
  vtkTypeMacro(vtkPVTimerInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the threshold to use to gather the timer log information. This must
  // be set before calling GatherInformation().
  vtkSetMacro(LogThreshold, double);
  vtkGetMacro(LogThreshold, double);

  // Description:
  // Access to the logs.
  int GetNumberOfLogs();
  char *GetLog(int proc);

  // Description:
  // Transfer information about a single object into
  // this object.
  virtual void CopyFromObject(vtkObject* data);
  virtual void CopyFromMessage(unsigned char* msg);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation* info);
  
  // Description: 
  // Serialize objects to/from a stream object.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream* css);

  // Description:
  // Serialize/Deserialize the parameters that control how/what information is
  // gathered. This are different from the ivars that constitute the gathered
  // information itself.
  virtual void CopyParametersToStream(vtkMultiProcessStream&);
  virtual void CopyParametersFromStream(vtkMultiProcessStream&);
protected:
  vtkPVTimerInformation();
  ~vtkPVTimerInformation();

  void Reallocate(int num);
  void InsertLog(int id, const char* log);

  double LogThreshold;
  int NumberOfLogs;
  char** Logs;

  vtkPVTimerInformation(const vtkPVTimerInformation&); // Not implemented
  void operator=(const vtkPVTimerInformation&); // Not implemented
};

#endif
