/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __vtkSQImageSource_h
#define __vtkSQImageSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkImageAlgorithm.h"
#include "CartesianExtent.h" // for CartesianExtent

class vtkInformation;
class vtkInformationVector;
class vtkDataSetAttributes;
class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQImageSource : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSQImageSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQImageSource *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set the whole extent of the generated dataset
  vtkGetVector6Macro(Extent,int);
  vtkSetVector6Macro(Extent,int);

  // Description:
  // For PV UI. Range domains only work with arrays of size 2.
  void SetIExtent(int ilo, int ihi);
  void SetJExtent(int jlo, int jhi);
  void SetKExtent(int klo, int khi);

  // Description:
  // Set the grid spacing.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);

  // Description:
  // Set the dataset origin
  vtkSetVector3Macro(Spacing,double);
  vtkGetVector3Macro(Spacing,double);

protected:
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQImageSource();
  virtual ~vtkSQImageSource();

private:
  int Extent[6];
  double Origin[3];
  double Spacing[3];

private:
  vtkSQImageSource(const vtkSQImageSource &); // Not implemented
  void operator=(const vtkSQImageSource &); // Not implemented
};

#endif
