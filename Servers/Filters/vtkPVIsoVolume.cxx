#include "vtkPVIsoVolume.h"

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

#include "vtkCompositeDataIterator.h"

#include <vtkstd/string>  // STL required.
#include <vtkstd/vector>  // STL required.

vtkCxxRevisionMacro(vtkPVIsoVolume, "1.1");
vtkStandardNewMacro(vtkPVIsoVolume);

const double PV_AMR_SURFACE_VALUE_UNSIGNED_CHAR=255;

//-----------------------------------------------------------------------------
class vtkPVIsoVolumeInternal
{
public:

  vtkstd::vector<vtkstd::string> CellArrays;
};

//-----------------------------------------------------------------------------
vtkPVIsoVolume::vtkPVIsoVolume() :
  vtkAMRDualClip(),
  VolumeFractionSurfaceValue(1.0)
{
  this->Implementation = new vtkPVIsoVolumeInternal();
}

//-----------------------------------------------------------------------------
vtkPVIsoVolume::~vtkPVIsoVolume()
{
  if(this->Implementation)
    {
    delete this->Implementation;
    this->Implementation = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVIsoVolume::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVIsoVolume::RequestData(vtkInformation* vtkNotUsed(request),
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet* hbdsInput=vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbdsOutput0 = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  this->SetIsoValue(this->VolumeFractionSurfaceValue *
                    PV_AMR_SURFACE_VALUE_UNSIGNED_CHAR);

  size_t noOfArrays = this->Implementation->CellArrays.size();
  for(size_t i=0; i < noOfArrays; ++i)
    {
    vtkMultiBlockDataSet* out = this->DoRequestData(
      hbdsInput, this->Implementation->CellArrays[i].c_str());

    if(out)
      {
      /// Assign the name to the block by the array name.
      mbdsOutput0->SetBlock(i, out);
      out->Delete();
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVIsoVolume::AddInputCellArrayToProcess(const char* name)
{
  this->Implementation->CellArrays.push_back(vtkstd::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVIsoVolume::ClearInputCellArrayToProcess()
{
  this->Implementation->CellArrays.clear();
  this->Modified();
}
