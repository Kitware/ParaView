#ifndef VTKCPPVSMPIPELINE_H
#define VTKCPPVSMPIPELINE_H

#include <string>
#include <vtkCPPipeline.h>

class vtkCPDataDescription;
class vtkCPPythonHelper;

class vtkCPPVSMPipeline : public vtkCPPipeline
{
public:
  static vtkCPPVSMPipeline* New();
  vtkTypeMacro(vtkCPPVSMPipeline, vtkCPPipeline);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize(int outputFrequency, std::string& fileName);

  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription);

  virtual int CoProcess(vtkCPDataDescription* dataDescription);

protected:
  vtkCPPVSMPipeline();
  virtual ~vtkCPPVSMPipeline();

private:
  vtkCPPVSMPipeline(const vtkCPPVSMPipeline&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCPPVSMPipeline&) VTK_DELETE_FUNCTION;

  int OutputFrequency;
  std::string FileName;
};
#endif
