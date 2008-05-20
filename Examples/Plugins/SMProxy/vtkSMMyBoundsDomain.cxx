
#include "vtkSMMyBoundsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"

vtkStandardNewMacro(vtkSMMyBoundsDomain)

vtkSMMyBoundsDomain::vtkSMMyBoundsDomain()
{
}

vtkSMMyBoundsDomain::~vtkSMMyBoundsDomain()
{
}

void vtkSMMyBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkSMMyBoundsDomain::Update(vtkSMProperty*)
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  
  vtkPVDataInformation* inputInformation = this->InputInformation;
  if (!this->InputInformation)
    {
    inputInformation = this->GetInputInformation();
    }
  if (!inputInformation)
    {
    return;
    }

  double bounds[6];
  inputInformation->GetBounds(bounds);

  // average the x, y
  double avgx = ( bounds[1] + bounds[0] ) / 2.0;
  double avgy = ( bounds[3] + bounds[2] ) / 2.0;

  this->AddMinimum(0, avgx);
  this->AddMinimum(1, avgy);
  this->AddMinimum(2, bounds[4]);
  
  this->AddMaximum(0, avgx);
  this->AddMaximum(1, avgy);
  this->AddMaximum(2, bounds[5]);
  
  this->InvokeModified();
}

