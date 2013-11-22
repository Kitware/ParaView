/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQHemisphereSource - Source/Reader that provides a polydata sphere as 2 hemispheres.
// .SECTION Description
// Source that provides a polydata sphere as 2 hemispheres on 2 outputs.
//

#ifndef __vtkSQHemisphereSource_h
#define __vtkSQHemisphereSource_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQHemisphereSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSQHemisphereSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQHemisphereSource *New();

  // Description:
  // Initialize from and xml document
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // Set/Get location of the sphere.
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Set/Get the vector along the north pole.
  vtkSetVector3Macro(North,double);
  vtkGetVector3Macro(North,double);

  // Description:
  // Set/Get the radius of the sphere.
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  // Description:
  // Set/Get the resolution (number of polys) used in the output.
  vtkSetMacro(Resolution,int);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set/Get descriptive names attached to each of the outputs.
  // The defaults are "north" and "south".
  vtkSetStringMacro(NorthHemisphereName);
  vtkGetStringMacro(NorthHemisphereName);
  vtkSetStringMacro(SouthHemisphereName);
  vtkGetStringMacro(SouthHemisphereName);

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  vtkSQHemisphereSource();
  ~vtkSQHemisphereSource();

  // VTK Pipeline
  int FillInputPortInformation(int port,vtkInformation *info);
  //int FillOutputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation* req, vtkInformationVector** input, vtkInformationVector* output);

private:
  vtkSQHemisphereSource(const vtkSQHemisphereSource&);  // Not implemented.
  void operator=(const vtkSQHemisphereSource&);  // Not implemented.

private:
  double North[3];
  double Center[3];
  double Radius;
  int Resolution;
  char *NorthHemisphereName;
  char *SouthHemisphereName;
  int LogLevel;
};

#endif
