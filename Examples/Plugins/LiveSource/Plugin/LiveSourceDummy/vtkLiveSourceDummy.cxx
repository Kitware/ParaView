#include <iostream>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLiveSourceDummy.h>

vtkStandardNewMacro(vtkLiveSourceDummy);

vtkLiveSourceDummy::vtkLiveSourceDummy()
{
  this->SetNumberOfInputPorts(0);
}

bool vtkLiveSourceDummy::GetNeedsUpdate()
{
  if (this->CurIteration < this->MaxIterations)
  {
    std::cout << "New iteration: " << this->CurIteration << std::endl;
    this->Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkLiveSourceDummy::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
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
  polydata->SetPolys(this->Cells);

  return 1;
}

// ----------------------------------------------------------------------------
void vtkLiveSourceDummy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "MaxIterations: " << this->MaxIterations << std::endl;
  os << "CurIteration:   " << this->CurIteration << std::endl;
}
