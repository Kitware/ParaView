/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQLogSource - A ParaView UI for the SciberQuest Log.
// .SECTION Description
// A dummy pipeline object (produces no data) providing a server
// manager UI for the SciberQuest Log. Applications may turn logging
// on and off view the GlobalLevel ivar. The data gathered by the log
// is written by the mpi root rank when this object is destroyed. Therefor
// this object must destroyed before MPI_Finalize is invoked.

#ifndef __vtkSQLogSource_h
#define __vtkSQLogSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQLogSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSQLogSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQLogSource *New();

  // Description:
  // Initialize from and xml document
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set/Get the global log level. Applications can enable logging
  // for all sciberquest objects by setting this.
  void SetGlobalLevel(int level);
  vtkGetMacro(GlobalLevel,int);

  // Description:
  // Set/Get the log file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkSQLogSource();
  ~vtkSQLogSource();

  // VTK Pipeline
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

private:
  vtkSQLogSource(const vtkSQLogSource&);  // Not implemented.
  void operator=(const vtkSQLogSource&);  // Not implemented.

private:
  int GlobalLevel;
  char *FileName;
};

#endif
