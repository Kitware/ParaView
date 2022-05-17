#include "vtkNastranBDFReader.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

vtkStandardNewMacro(vtkNastranBDFReader);

static const std::string COMMENT_KEY = "$";
static const std::vector<std::string> IGNORED_KEYS = { COMMENT_KEY, "BEGIN BULK", "ENDDATA",
  "PSHELL", "MAT1" };

static const std::string CTRIA3_KEY = "CTRIA3";
static const std::string GRID_KEY = "GRID";
static const std::string PLOAD2_KEY = "PLOAD2";
static const std::string TIME_KEY = "TIME";
static const std::string TITLE_KEY = "TITLE";

namespace utils
{
// Returns if `line` starts with the string `keyword`
bool StartsWith(const std::string& line, const std::string& keyword)
{
  return line.rfind(keyword, 0) == 0;
}

// Returns if `line` matches a keyword that should be silently ignored
bool IsIgnored(const std::string& line)
{
  for (const auto& ignoring : IGNORED_KEYS)
  {
    if (StartsWith(line, ignoring))
    {
      return true;
    }
  }
  return false;
}

// In-place removal of trailing comment
void TrimTrailingComment(std::string& line)
{
  if (auto pos = line.find(COMMENT_KEY) != std::string::npos)
  {
    line.erase(pos);
  }
}

// Parses a whole `line`, depending on its starting `keyword`.
// Returns a vector of values as string.
// Typically, it splits the line around each delimiter `,`.
std::vector<std::string> ParseArgs(const std::string& line, const std::string& keyword)
{
  std::vector<std::string> arguments;
  if (keyword == TITLE_KEY)
  {
    // remove `TITLE` keyword and the `=` char
    arguments.push_back(line.substr(line.find(keyword) + keyword.size() + 1));
  }
  else if (keyword == TIME_KEY)
  {
    arguments.push_back(line.substr(line.find(keyword) + keyword.size()));
  }
  else
  {
    std::stringstream linestream(line);
    std::string arg;
    // first element is the keyword, do not store it as arg.
    std::getline(linestream, arg, ',');
    while (std::getline(linestream, arg, ','))
    {
      arguments.push_back(arg);
    }
  }

  return arguments;
}
};

//------------------------------------------------------------------------------
vtkNastranBDFReader::vtkNastranBDFReader()
{
  this->SetNumberOfInputPorts(0);
  this->OriginalPointIds->SetName("Ids");
}

//------------------------------------------------------------------------------
vtkIdType vtkNastranBDFReader::GetVTKPointId(const std::string& arg)
{
  vtkIdType elt = std::stol(arg);
  return this->PointsIds.find(elt) != this->PointsIds.end() ? this->PointsIds.find(elt)->second
                                                            : -1;
}

//------------------------------------------------------------------------------
bool vtkNastranBDFReader::AddTitle(const std::vector<std::string>& args)
{
  vtkNew<vtkStringArray> data;
  data->SetName(TITLE_KEY.c_str());
  data->InsertNextValue(args[0]);
  this->GetOutput()->GetFieldData()->AddArray(data);
  return true;
}

//------------------------------------------------------------------------------
bool vtkNastranBDFReader::AddTimeInfo(const std::vector<std::string>& args)
{
  vtkNew<vtkDoubleArray> data;
  data->SetName(TIME_KEY.c_str());
  data->InsertNextValue(std::stod(args[0]));
  this->GetOutput()->GetFieldData()->AddArray(data);
  return true;
}

//------------------------------------------------------------------------------
bool vtkNastranBDFReader::AddPoint(const std::vector<std::string>& args)
{
  // Expected args: ID CP X1 X2 X3.
  // Where:
  // ID is the id of the point,
  // CP is unused,
  // X1, X2, X3 are coordinates
  // extra args are silently ignored
  if (args.size() < 5)
  {
    vtkErrorMacro("Wrong size for GRID element, should be at least 5");
    return false;
  }

  vtkIdType originalId = std::stod(args[0]);
  vtkIdType id =
    this->Points->InsertNextPoint(std::stod(args[2]), std::stod(args[3]), std::stod(args[4]));
  this->OriginalPointIds->InsertNextValue(originalId);
  this->PointsIds[originalId] = id;

  return true;
}

//------------------------------------------------------------------------------
bool vtkNastranBDFReader::AddTriangle(const std::vector<std::string>& args)
{
  // Expected args: EID PID G1 G2 G3
  // Where:
  // EID is the corresponding cell id,
  // PID is unused,
  // G1 G2 G3 are points ids defining a triangle.
  // extra args are silently ignored
  if (args.size() < 5)
  {
    vtkErrorMacro("Wrong size for CTRIA3 element, should be at least 5");
    return false;
  }

  vtkNew<vtkTriangle> tri;
  vtkIdType pt1Id = this->GetVTKPointId(args[2]);
  vtkIdType pt2Id = this->GetVTKPointId(args[3]);
  vtkIdType pt3Id = this->GetVTKPointId(args[4]);

  if (pt1Id == -1 || pt2Id == -1 || pt3Id == -1)
  {
    vtkErrorMacro(<< "Undefined point in triangle (" << args[2] << ", " << args[3] << ", "
                  << args[4] << ")");
    return false;
  }

  tri->GetPointIds()->SetId(0, pt1Id);
  tri->GetPointIds()->SetId(1, pt2Id);
  tri->GetPointIds()->SetId(2, pt3Id);

  if (this->Cells->GetNumberOfCells() == 0)
  {
    this->Cells->AllocateEstimate(this->Points->GetNumberOfPoints(), 3);
  }

  vtkIdType id = this->Cells->InsertNextCell(tri);
  this->CellsIds[std::stol(args[0])] = id;

  return true;
}

//------------------------------------------------------------------------------
bool vtkNastranBDFReader::AddPload2Data(const std::vector<std::string>& args)
{
  // Expected args: SID P EID
  // SID: Load set identification number (unused).
  // P: Pressure value
  // EID: Element identification number.
  // extra args are silently ignored
  if (args.size() < 3)
  {
    vtkErrorMacro("Wrong size for PLOAD2 element, should be at least 3");
    return false;
  }

  if (this->CellsIds.empty())
  {
    vtkErrorMacro("Trying to add PLOAD2 data without any cell defined. Skipping.");
    return false;
  }

  if (!this->Pload2)
  {
    this->Pload2 = vtkSmartPointer<vtkDoubleArray>::New();
    this->Pload2->SetNumberOfTuples(this->Cells->GetNumberOfCells());
    this->Pload2->SetName(PLOAD2_KEY.c_str());
  }

  this->Pload2->InsertValue(this->CellsIds[std::stol(args[2])], std::stod(args[1]));
  return true;
}

//------------------------------------------------------------------------------
int vtkNastranBDFReader::RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  // Create stream
  vtksys::ifstream filestream;
  vtksys::SystemTools::Stat_t fs;
  if (!vtksys::SystemTools::Stat(this->FileName, &fs))
  {
    filestream.open(this->FileName.c_str(), ios::in);
  }
  if (filestream.fail())
  {
    vtkErrorMacro("Could not open file : " << this->FileName);
    return false;
  }

  // read lines, and call appropriate method depending on keyword
  std::string line;
  bool success = true;
  while (vtksys::SystemTools::GetLineFromStream(filestream, line) && success)
  {
    // skip blank and comments
    if (line.empty() || utils::IsIgnored(line))
    {
      continue;
    }

    utils::TrimTrailingComment(line);

    // we parse values with std::sto[ild] that may raise exception. Catch them.
    try
    {
      // parse line depending on keyword
      if (utils::StartsWith(line, TITLE_KEY))
      {
        success = this->AddTitle(utils::ParseArgs(line, TITLE_KEY));
      }
      else if (utils::StartsWith(line, TIME_KEY))
      {
        success = this->AddTimeInfo(utils::ParseArgs(line, TITLE_KEY));
      }
      else if (utils::StartsWith(line, GRID_KEY))
      {
        success = this->AddPoint(utils::ParseArgs(line, GRID_KEY));
      }
      else if (utils::StartsWith(line, CTRIA3_KEY))
      {
        success = this->AddTriangle(utils::ParseArgs(line, CTRIA3_KEY));
      }
      else if (utils::StartsWith(line, PLOAD2_KEY))
      {
        success = this->AddPload2Data(utils::ParseArgs(line, PLOAD2_KEY));
      }
      // store unsupported keyword for summary reporting.
      else
      {
        std::string keyword = line.substr(0, line.find(','));
        if (this->UnsupportedElements.find(keyword) != this->UnsupportedElements.end())
        {
          this->UnsupportedElements[keyword]++;
        }
        else
        {
          this->UnsupportedElements[keyword] = 1;
        }
      }
    }
    catch (std::invalid_argument const& ex)
    {
      vtkErrorMacro(<< "Error while parsing number, wrong type: " << ex.what());
      success = false;
    }
    catch (std::out_of_range const& ex)
    {
      vtkErrorMacro(<< "Error while parsing number, out of range: " << ex.what());
      success = false;
    }
  }

  if (!success)
  {
    vtkErrorMacro(<< "Fail to read file."
                  << "\n"
                  << "Error with line: \n"
                  << line);
    return 0;
  }

  for (const auto& unsupported : this->UnsupportedElements)
  {
    vtkWarningMacro("Skip unsupported entry `" << unsupported.first << "` (" << unsupported.second
                                               << " occurences)");
  }

  auto output = this->GetOutput();
  output->SetPoints(this->Points);
  output->GetPointData()->AddArray(this->OriginalPointIds);
  output->SetCells(VTK_TRIANGLE, this->Cells);
  if (this->Pload2)
  {
    output->GetCellData()->AddArray(this->Pload2);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkNastranBDFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName.empty() ? this->FileName : "(none)") << endl;
}
