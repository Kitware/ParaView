#include "vtkPVAMRDualClip.h"

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

#include "vtkCompositeDataIterator.h"

#include <vtkstd/string>  // STL required.
#include <vtkstd/vector>  // STL required.

vtkStandardNewMacro(vtkPVAMRDualClip);

const double PV_AMR_SURFACE_VALUE_UNSIGNED_CHAR=255;

//-----------------------------------------------------------------------------
class vtkPVAMRDualClipInternal
{
public:

  vtkstd::vector<vtkstd::string> CellArrays;
};

//-----------------------------------------------------------------------------
vtkPVAMRDualClip::vtkPVAMRDualClip() :
  vtkAMRDualClip(),
  VolumeFractionSurfaceValue(1.0)
{
  this->Implementation = new vtkPVAMRDualClipInternal();
}

//-----------------------------------------------------------------------------
vtkPVAMRDualClip::~vtkPVAMRDualClip()
{
  if(this->Implementation)
    {
    delete this->Implementation;
    this->Implementation = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVAMRDualClip::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVAMRDualClip::RequestData(vtkInformation* vtkNotUsed(request),
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


  // Don't call SetIsoValue() that changes filter's MTime which is not
  // acceptable in RequestData().
  this->IsoValue = (this->VolumeFractionSurfaceValue *
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
void vtkPVAMRDualClip::AddInputCellArrayToProcess(const char* name)
{
  this->Implementation->CellArrays.push_back(vtkstd::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRDualClip::ClearInputCellArrayToProcess()
{
  this->Implementation->CellArrays.clear();
  this->Modified();
}
