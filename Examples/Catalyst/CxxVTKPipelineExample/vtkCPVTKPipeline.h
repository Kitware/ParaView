#ifndef VTKCPVTKPIPELINE_H
#define VTKCPVTKPIPELINE_H

#include <string>
#include <vtkCPPipeline.h>

class vtkCPDataDescription;
class vtkCPPythonHelper;

class vtkCPVTKPipeline : public vtkCPPipeline
{
public:
  static vtkCPVTKPipeline* New();
  vtkTypeMacro(vtkCPVTKPipeline, vtkCPPipeline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Initialize(int outputFrequency, std::string& fileName);

  int RequestDataDescription(vtkCPDataDescription* dataDescription) override;

  int CoProcess(vtkCPDataDescription* dataDescription) override;

protected:
  vtkCPVTKPipeline();
  virtual ~vtkCPVTKPipeline();

private:
  vtkCPVTKPipeline(const vtkCPVTKPipeline&) = delete;
  void operator=(const vtkCPVTKPipeline&) = delete;

  int OutputFrequency;
  std::string FileName;
};
#endif
