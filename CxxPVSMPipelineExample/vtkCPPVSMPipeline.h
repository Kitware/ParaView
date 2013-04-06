#ifndef VTKCPPVSMPIPELINE_H
#define VTKCPPVSMPIPELINE_H

#include <vtkCPPipeline.h>
#include <string>

class vtkCPDataDescription;
class vtkCPPythonHelper;

class vtkCPPVSMPipeline : public vtkCPPipeline
{
public:
  static vtkCPPVSMPipeline* New();
  vtkTypeMacro(vtkCPPVSMPipeline,vtkCPPipeline);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize(int outputFrequency, std::string& fileName);

  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription);

  virtual int CoProcess(vtkCPDataDescription* dataDescription);

protected:
  vtkCPPVSMPipeline();
  virtual ~vtkCPPVSMPipeline();

private:
  vtkCPPVSMPipeline(const vtkCPPVSMPipeline&); // Not implemented
  void operator=(const vtkCPPVSMPipeline&); // Not implemented

  int OutputFrequency;
  std::string FileName;
};
#endif
