
#include "vtkSMMyBoundsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"

vtkStandardNewMacro(vtkSMMyBoundsDomain);

vtkSMMyBoundsDomain::vtkSMMyBoundsDomain() = default;

vtkSMMyBoundsDomain::~vtkSMMyBoundsDomain() = default;

void vtkSMMyBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkSMMyBoundsDomain::Update(vtkSMProperty*)
{
  vtkPVDataInformation* inputInformation = this->GetInputInformation();
  if (!inputInformation)
  {
    return;
  }

  double bounds[6];
  inputInformation->GetBounds(bounds);

  // average the x, y
  double avgx = (bounds[1] + bounds[0]) / 2.0;
  double avgy = (bounds[3] + bounds[2]) / 2.0;

  std::vector<vtkEntry> entries;
  entries.push_back(vtkEntry(avgx, avgx));
  entries.push_back(vtkEntry(avgy, avgy));
  entries.push_back(vtkEntry(bounds[4], bounds[5]));

  this->SetEntries(entries);
}
