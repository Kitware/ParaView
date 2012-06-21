/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQAgyrotropyFilter -
// .SECTION Description
//

#ifndef __vtkSQAgyrotropyFilter_h
#define __vtkSQAgyrotropyFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkPVXMLElement;

class VTK_EXPORT vtkSQAgyrotropyFilter : public vtkDataSetAlgorithm
{
public:
  static vtkSQAgyrotropyFilter *New();

  vtkTypeMacro(vtkSQAgyrotropyFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the object from an xml document.
  int Initialize(vtkPVXMLElement *root);


protected:
  vtkSQAgyrotropyFilter();
  ~vtkSQAgyrotropyFilter();

  // VTK Pipeline
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

private:
  vtkSQAgyrotropyFilter(const vtkSQAgyrotropyFilter&);  // Not implemented.
  void operator=(const vtkSQAgyrotropyFilter&);  // Not implemented.

private:
};

#endif
