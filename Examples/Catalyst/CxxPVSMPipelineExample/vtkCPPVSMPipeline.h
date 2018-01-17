#ifndef VTKCPPVSMPIPELINE_H
#define VTKCPPVSMPIPELINE_H

#include <string>
#include <vtkCPPipeline.h>

class vtkCPPVSMPipeline : public vtkCPPipeline
{
public:
  static vtkCPPVSMPipeline* New();
  vtkTypeMacro(vtkCPPVSMPipeline, vtkCPPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Initialize(int outputFrequency, std::string& fileName);

  int RequestDataDescription(vtkCPDataDescription* dataDescription) override;

  int CoProcess(vtkCPDataDescription* dataDescription) override;

protected:
  vtkCPPVSMPipeline();
  virtual ~vtkCPPVSMPipeline();

private:
  vtkCPPVSMPipeline(const vtkCPPVSMPipeline&) = delete;
  void operator=(const vtkCPPVSMPipeline&) = delete;

  int OutputFrequency;
  std::string FileName;
};
#endif
