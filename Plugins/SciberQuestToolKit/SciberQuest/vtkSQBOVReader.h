/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQBOVReader -- Connects the VTK pipeline to BOVReader class.
// .SECTION Description
//
// Implements the VTK style pipeline and manipulates and instance of
// BOVReader so that "brick of values" datasets, including time series,
// can be read in parallel.
//
// .SECTION See Also
// BOVReader

#ifndef __vtkSQBOVReader_h
#define __vtkSQBOVReader_h
// #define vtkSQBOVReaderDEBUG

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkSQBOVReaderBase.h"

#include <vector> // for vector
#include <string> // for string

//BTX
class vtkPVXMLElement;
//ETX

class VTKSCIBERQUEST_EXPORT vtkSQBOVReader : public vtkSQBOVReaderBase
{
public:
  static vtkSQBOVReader *New();
  vtkTypeMacro(vtkSQBOVReader,vtkSQBOVReaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Iitialize the reader from an XML document. You also need to
  // pass in the bov file name so that subsetting and array selection
  // can be applied which has to occur after the file has been opened.
  //BTX
  virtual int Initialize(
        vtkPVXMLElement *root,
        const char *fileName,
        std::vector<std::string> &arrays);
  //ETX

protected:
  virtual int RequestInformation(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  virtual int RequestData(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos);

  vtkSQBOVReader();
  virtual ~vtkSQBOVReader();

  virtual void Clear();

private:
  vtkSQBOVReader(const vtkSQBOVReader &); // Not implemented
  void operator=(const vtkSQBOVReader &); // Not implemented

private:

};

#endif
