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
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Initialize(int outputFrequency, std::string& fileName);

  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription);

  virtual int CoProcess(vtkCPDataDescription* dataDescription);

protected:
  vtkCPVTKPipeline();
  virtual ~vtkCPVTKPipeline();

private:
  vtkCPVTKPipeline(const vtkCPVTKPipeline&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCPVTKPipeline&) VTK_DELETE_FUNCTION;

  int OutputFrequency;
  std::string FileName;
};
#endif
