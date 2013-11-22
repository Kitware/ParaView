/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQFTLE
// .SECTION Description
// Compute the FTLE given a displacement map. For an explanation of this
// terminology see http://amath.colorado.edu/cmsms/index.php?page=ftle-of-the-standard-map
//
// .SECTION Caveats
// .SECTION See Also

#ifndef __vtkSQFTLE_h
#define __vtkSQFTLE_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

#include <set> // for set
#include <string> // for string

class vtkInformation;
class vtkInformationVector;
class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQFTLE : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQFTLE,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct to compute the gradient of the scalars and vectors.
  static vtkSQFTLE *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Array selection.
  void AddInputArray(const char *name);
  void ClearInputArrays();

  // Description:
  // Shallow copy input data arrays to the output.
  vtkSetMacro(PassInput,int);
  vtkGetMacro(PassInput,int);

  // Description:
  // Set/Get the time interval overwhich displacement map was
  // integrated.
  vtkSetMacro(TimeInterval,double);
  vtkGetMacro(TimeInterval,double);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQFTLE();
  ~vtkSQFTLE(){}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  std::set<std::string> InputArrays;
  int PassInput;
  double TimeInterval;
  int LogLevel;

private:
  vtkSQFTLE(const vtkSQFTLE&);  // Not implemented.
  void operator=(const vtkSQFTLE&);  // Not implemented.
};

#endif
