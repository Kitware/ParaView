#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLiveSourceDummy.h>
#include <vtkPoints.h>
#include <vtkStreamingDemandDrivenPipeline.h>

vtkStandardNewMacro(vtkLiveSourceDummy);

//------------------------------------------------------------------------------
vtkLiveSourceDummy::vtkLiveSourceDummy()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
bool vtkLiveSourceDummy::GetNeedsUpdate()
{
  if (this->CurIteration < this->MaxIterations)
  {
    this->Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkLiveSourceDummy::RequestData(vtkInformation*, vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* polydata = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->Points->InsertNextPoint(this->Pos.a, this->Pos.b, this->Pos.c);
  polydata->SetPoints(this->Points);

  this->Pos = this->Tri.next(this->Pos);

  this->Cells->InsertNextCell(1);
  this->Cells->InsertCellPoint(this->CurIteration++);
  polydata->SetVerts(this->Cells);

  return 1;
}

// ----------------------------------------------------------------------------
void vtkLiveSourceDummy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "MaxIterations: " << this->MaxIterations << std::endl;
  os << "CurIteration:   " << this->CurIteration << std::endl;
}
