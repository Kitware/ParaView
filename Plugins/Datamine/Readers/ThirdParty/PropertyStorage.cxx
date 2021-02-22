// ->NAME vtkDataMineWireFrameReader->cxx
// Read DataMine binary files for single objects->
// point, perimeter (polyline), wframe<points/triangle>
// With or without properties (each property name < 8 characters)
// The binary file reading is done by 'dmfile->cxx'
//     99-04-12: Written by Jeremy Maccelari, visualn@iafrica->com
#include "PropertyStorage.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"

// --------------------------------------
PropertyItem::PropertyItem(
  const std::string& aname, const bool& numeric, const int& pos, const int& status, int numRecords)
{
  this->isNumeric = numeric;

  this->name = aname;
  // right trim the name
  size_t endpos = this->name.find_last_not_of(" \t");
  if (std::string::npos != endpos)
  {
    this->name = this->name.substr(0, endpos + 1);
  }

  this->startPos = pos;
  this->endPos = pos + 1; // will be updated by AddProperty

  // setup the defaults before we run the case statement
  // notice case 2: is a fall through
  this->isSegmentable = false;
  this->isActive = false;
  this->Storage = nullptr;
  switch (status)
  {
    case 2:
      this->isSegmentable = true;
      VTK_FALLTHROUGH;
    case 1:
      this->isActive = true;
      if (this->isNumeric)
      {
        this->Storage.TakeReference(vtkDoubleArray::New());
      }
      else
      {
        this->Storage.TakeReference(vtkStringArray::New());
      }
      this->Storage->Allocate(numRecords);
      this->Storage->SetName(this->name.c_str());
      break;
    case 0:
    default:
      break;
  }
}
PropertyItem::~PropertyItem() = default;

// --------------------------------------
PropertyStorage::PropertyStorage() = default;

// --------------------------------------
PropertyStorage::~PropertyStorage() = default;

// --------------------------------------
void PropertyStorage::AddProperty(
  char* name, const bool& numeric, const int& pos, const int& status, int numRecords)
{
  std::string vname(name);
  if (this->properties.size() > 0)
  {
    PropertyItem& lastItem = this->properties.back();
    // check the last item in properties. We check both name and postion continunity to confirm if
    // we need to expand the item or create a new item

    // note: lastItem->name is trimmed, so find on vname
    if (vname.find(lastItem.name) == 0 && pos == lastItem.endPos)
    {
      // item is continous
      lastItem.endPos++;
      return;
    }
  }

  // this is hit on the first item, and even new item afterwards
  this->properties.push_back(PropertyItem(vname, numeric, pos, status, numRecords));
}

// --------------------------------------
void PropertyStorage::AddValues(Data* values)
{
  for (auto& item : this->properties)
  {
    if (item.isActive) // ignore non active
    {
      if (item.isNumeric)
      {
        static_cast<vtkDoubleArray*>(item.Storage.Get())->InsertNextValue(values[item.startPos].v);
      }
      else
      {
        char ctmp[5];
        ctmp[4] = 0;
        std::string tempBuf;
        for (int pos = item.startPos; pos < item.endPos; ++pos)
        {
          ctmp[0] = values[pos].c[0];
          ctmp[1] = values[pos].c[1];
          ctmp[2] = values[pos].c[2];
          ctmp[3] = values[pos].c[3];
          tempBuf += ctmp;
        }
        static_cast<vtkStringArray*>(item.Storage.Get())->InsertNextValue(tempBuf);
      }
    }
  }
}

// --------------------------------------
void PropertyStorage::Segment(const int& records)
{
  // only support the ability to segment a double property
  double value;
  unsigned int pos, end;
  for (auto& item : this->properties)
  {
    if (item.isSegmentable && item.isNumeric)
    {
      vtkDoubleArray* da = static_cast<vtkDoubleArray*>(item.Storage.Get());
      end = da->GetNumberOfValues();
      for (pos = end - records; pos < end; ++pos)
      {
        value = da->GetValue(pos);
        value /= records;
        da->SetValue(pos, value);
      }
    }
  }
}

// --------------------------------------
void PropertyStorage::PushToDataSet(vtkDataSet* dataSet)
{
  // add the properties to the dataset, we detect if the property is point or cell based
  // If both(point&cell) are the same it goes to points
  vtkIdType size;
  vtkIdType numPoints = dataSet->GetNumberOfPoints();
  for (auto& item : this->properties)
  {
    if (!item.isActive)
    {
      continue;
    }
    size = item.Storage->GetNumberOfValues();
    if (numPoints == size)
    {
      if (dataSet->GetPointData()->GetAbstractArray(item.name.c_str()) == nullptr)
      {
        dataSet->GetPointData()->AddArray(item.Storage);
      }
    }
    else
    {
      if (dataSet->GetCellData()->GetAbstractArray(item.name.c_str()) == nullptr)
      {
        dataSet->GetCellData()->AddArray(item.Storage);
      }
    }
  }
}
