/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQFTLE
// .SECTION Description
// Compute the FTLE given a displacement map. For  an explanation of this
// terminology see http://amath.colorado.edu/cmsms/index.php?page=ftle-of-the-standard-map
//
// .SECTION Caveats
// .SECTION See Also

#ifndef __vtkSQFTLE_h
#define __vtkSQFTLE_h

#include "vtkDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkPVXMLElement;

class VTK_EXPORT vtkSQFTLE : public vtkDataSetAlgorithm
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
  // Deep copy input arrays to the output.
  vtkSetMacro(PassInput,int);
  vtkGetMacro(PassInput,int);

protected:
  vtkSQFTLE();
  ~vtkSQFTLE(){}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  // controls to turn on/off array generation
  int PassInput;

private:
  vtkSQFTLE(const vtkSQFTLE&);  // Not implemented.
  void operator=(const vtkSQFTLE&);  // Not implemented.
};

#endif
