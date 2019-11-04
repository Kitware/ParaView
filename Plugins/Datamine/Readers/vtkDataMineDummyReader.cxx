// .NAME DataMineDummyReader.cxx
// Read DataMine binary files for single objects.
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile.cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica.com

#include "vtkDataMineDummyReader.h"
#include "dmfile.h"

#include "vtkCallbackCommand.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkDataMineDummyReader);

// Constructor
vtkDataMineDummyReader::vtkDataMineDummyReader()
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// --------------------------------------
// Destructor
vtkDataMineDummyReader::~vtkDataMineDummyReader()
{
  this->SetFileName(0);
}

void vtkDataMineDummyReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// --------------------------------------
int vtkDataMineDummyReader::CanReadFile(const char* fname)
{
  // okay we have to un constant the char* so that TDMFile can use it
  // TDMFile does not modify the fname so this is 'okay'
  if (fname == NULL || fname[0] == '\0' || strcmp(fname, " ") == 0)
    return 0;
  char* tmpName = const_cast<char*>(fname);

  // load the File
  TDMFile* dmFile = new TDMFile();
  dmFile->LoadFileHeader(tmpName);

  // Get File Type
  FileTypes filetype = dmFile->GetFileType();

  // make sure we only return true to the formats we don't
  // actually support somewhere else

  const int supportedSize = 7;
  FileTypes supported[supportedSize] = { perimeter, drillhole, blockmodel, wframetriangle,
    wframepoints, point, stopesummary };

  // using a magic number since don't want to create a dynamic array
  // since we will have to modify this file anyhow each time we add a supported file type
  // currently we have 6 supported and 12 unsupported
  int result = 1; // we support dm files that we nothing else reads
  for (int i = 0; i < supportedSize; i++)
  {
    if (filetype == supported[i])
    {
      result = 0; // let the proper reader read the file
    }
  }

  delete dmFile;
  return result;
}

// --------------------------------------
int vtkDataMineDummyReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  vtkErrorMacro("We currently do not support this DataMine format");
  return 1;
}
