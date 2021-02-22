#include "vtkSpyPlotReaderMap.h"
#include "vtkMultiProcessStream.h"
#include "vtkSpyPlotReader.h"
#include "vtkSpyPlotUniReader.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <assert.h>

namespace
{
// tests if the filename specified has a numerical extension.
// If so, returns the number specified in the extension as "number".
bool HasNumericalExtension(const char* filename, int& number)
{
  number = 0;

  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(filename);
  // NOTE: GetFilenameWithoutLastExtension() returns the extension with "."
  // included.
  if (extension.size() > 1)
  {
    const char* a = extension.c_str();
    a++; // to exclude the "."
    if (isdigit(*a))
    {
      char* ep;
      number = static_cast<int>(strtol(a, &ep, 10));
      if (ep[0] == '\0')
      {
        return true;
      }
    }
  }
  return false;
}
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReaderMap::Clean(vtkSpyPlotUniReader* save)
{
  MapOfStringToSPCTH::iterator it;
  MapOfStringToSPCTH::iterator end = this->Files.end();
  for (it = this->Files.begin(); it != end; ++it)
  {
    if (it->second && it->second != save)
    {
      it->second->Delete();
      it->second = 0;
    }
  }
  this->Files.erase(this->Files.begin(), end);
}

//-----------------------------------------------------------------------------
bool vtkSpyPlotReaderMap::Save(vtkMultiProcessStream& stream)
{
  stream << 12345 << static_cast<int>(this->Files.size());
  for (MapOfStringToSPCTH::iterator iter = this->Files.begin(); iter != this->Files.end(); ++iter)
  {
    stream << iter->first;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSpyPlotReaderMap::Load(vtkMultiProcessStream& stream)
{
  this->Clean(nullptr);
  int size, magic_number;
  stream >> magic_number >> size;
  assert(magic_number == 12345);
  for (int cc = 0; cc < size; cc++)
  {
    std::string fname;
    stream >> fname;
    this->Files[fname] = nullptr;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSpyPlotReaderMap::Initialize(const char* filename)
{
  this->Clean(nullptr);

  vtksys::ifstream ifs(filename);
  if (!ifs)
  {
    vtkGenericWarningMacro("Error opening file " << filename);
    return false;
  }

  char buffer[8];
  if (!ifs.read(buffer, 7))
  {
    vtkGenericWarningMacro("Problem reading header of file: " << filename);
    return false;
  }
  buffer[7] = 0;
  ifs.close();

  // Build the file map by processing basic meta-data from the file i.e. if it's
  // a case file, use the files specified. If it's a spcth data file, test to
  // see if there are multiple files in the same location.
  if (strcmp(buffer, "spydata") == 0)
  {
    return this->InitializeFromSpyFile(filename);
  }
  else if (strcmp(buffer, "spycase") == 0)
  {
    return this->InitializeFromCaseFile(filename);
  }

  vtkGenericWarningMacro("Not a SpyData file");
  return false;
}

//-----------------------------------------------------------------------------
vtkSpyPlotUniReader* vtkSpyPlotReaderMap::GetReader(
  MapOfStringToSPCTH::iterator& it, vtkSpyPlotReader* parent)
{
  if (!it->second)
  {
    it->second = vtkSpyPlotUniReader::New();
    it->second->SetCellArraySelection(parent->GetCellDataArraySelection());
    it->second->SetFileName(it->first.c_str());
    // cout << parent->GetController()->GetLocalProcessId()
    // << "Create reader: " << it->second << endl;
  }
  return it->second;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotReaderMap::TellReadersToCheck(vtkSpyPlotReader* parent)
{
  MapOfStringToSPCTH::iterator it;
  MapOfStringToSPCTH::iterator end = this->Files.end();
  for (it = this->Files.begin(); it != end; ++it)
  {
    this->GetReader(it, parent)->SetNeedToCheck(1);
  }
}

//-----------------------------------------------------------------------------
bool vtkSpyPlotReaderMap::InitializeFromSpyFile(const char* filename)
{
  // cerr << "spydatafile\n";
  // See if this is part of a series
  int currentNum = 0;
  bool isASeries = HasNumericalExtension(filename, currentNum);

  if (!isASeries)
  {
    // Add an entry in the Map. The reader will be created on-demand when
    // needed.
    this->Files[filename] = nullptr;
    return true;
  }

  // There's a possibility that this is a spy file series. Try to detect the
  // series.
  std::string fileNoExt = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
  std::string filePath = vtksys::SystemTools::GetFilenamePath(filename);

  // Now find all the files that make up the series that this file is part
  // of
  int idx = currentNum - 100;
  int last = currentNum;
  char buffer[1024];
  int found = currentNum;
  int minimum = currentNum;
  int maximum = currentNum;
  while (1)
  {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    // cerr << "buffer1 == " << buffer << endl;
    if (!vtksys::SystemTools::FileExists(buffer))
    {
      int next = idx;
      for (idx = last; idx > next; idx--)
      {
        sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
        if (!vtksys::SystemTools::FileExists(buffer))
        {
          break;
        }
        else
        {
          found = idx;
        }
      }
      break;
    }
    last = idx;
    idx -= 100;
  }
  minimum = found;
  idx = currentNum + 100;
  last = currentNum;
  found = currentNum;
  while (1)
  {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    // cerr << "buffer2 == " << buffer << endl;
    if (!vtksys::SystemTools::FileExists(buffer))
    {
      int next = idx;
      for (idx = last; idx < next; idx++)
      {
        sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
        if (!vtksys::SystemTools::FileExists(buffer))
        {
          break;
        }
        else
        {
          found = idx;
        }
      }
      break;
    }
    last = idx;
    idx += 100;
  }
  maximum = found;
  for (idx = minimum; idx <= maximum; ++idx)
  {
    sprintf(buffer, "%s/%s.%d", filePath.c_str(), fileNoExt.c_str(), idx);
    // cerr << "buffer3 == " << buffer << endl;
    this->Files[buffer] = nullptr;
  }
  // Okay now open just the first file to get meta data
  // cerr << "updating meta... " << endl;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSpyPlotReaderMap::InitializeFromCaseFile(const char* filename)
{
  // Setup the filemap and spcth structures
  vtksys::ifstream ifs(filename);
  if (!ifs)
  {
    vtkGenericWarningMacro("Error opening file " << filename);
    return false;
  }

  std::string line;
  if (!vtksys::SystemTools::GetLineFromStream(ifs, line)) // eat spycase line
  {
    vtkGenericWarningMacro("Syntax error in case file " << filename);
    return 0;
  }

  while (vtksys::SystemTools::GetLineFromStream(ifs, line))
  {
    if (line.length() != 0) // Skip blank lines
    {
      std::string::size_type stp = line.find_first_not_of(" \n\t\r");
      std::string::size_type etp = line.find_last_not_of(" \n\t\r");
      std::string f(line, stp, etp - stp + 1);
      if (f[0] != '#') // skip comment
      {
        if (!vtksys::SystemTools::FileIsFullPath(f.c_str()))
        {
          f = vtksys::SystemTools::GetFilenamePath(filename) + "/" + f;
        }
        this->Files[f.c_str()] = nullptr;
      }
    }
  }

  return true;
}
