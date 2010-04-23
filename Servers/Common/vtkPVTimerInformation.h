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
// I am using this infomration object to gather timer logs from all processes.

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
protected:
  vtkPVTimerInformation();
  ~vtkPVTimerInformation();

  void Reallocate(int num);
  void InsertLog(int id, const char* log);

  int NumberOfLogs;
  char** Logs;

  vtkPVTimerInformation(const vtkPVTimerInformation&); // Not implemented
  void operator=(const vtkPVTimerInformation&); // Not implemented
};

#endif
