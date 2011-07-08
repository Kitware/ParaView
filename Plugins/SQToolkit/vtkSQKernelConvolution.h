/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __vtkSQKernelConvolution_h
#define __vtkSQKernelConvolution_h

#include "vtkDataSetAlgorithm.h"
#include "CartesianExtent.h"

class vtkInformation;
class vtkInformationVector;



class vtkSQKernelConvolution : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQKernelConvolution,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQKernelConvolution *New();

  //BTX
  enum {
    MODE_3D=0,
    MODE_2D_XY,
    MODE_2D_XZ,
    MODE_2D_YZ
    };
  //ETX
  // Description:
  // Set the mode to 2 or 3D.
  void SetMode(int mode);
  vtkGetMacro(Mode,int);

  //BTX
  enum {
    KERNEL_TYPE_GAUSIAN=0,
    KERNEL_TYPE_CONSTANT=1
    };
  //ETX
  // Description:
  // Select a kernel for the convolution.
  void SetKernelType(int type);
  vtkGetMacro(KernelType,int);

  // Description:
  // Set the stencil width, must be an odd integer, bound bellow by 3
  // and above by the size of the smallest block
  void SetKernelWidth(int width);
  vtkGetMacro(KernelWidth,int);

  // Description:
  // Set the number of itterations to apply. NOT IMPLEMENTED.
  vtkSetMacro(NumberOfIterations,int);
  vtkGetMacro(NumberOfIterations,int);

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQKernelConvolution();
  virtual ~vtkSQKernelConvolution();

  // Description:
  // Called before execution to generate the selected kernel.
  int UpdateKernel();

private:
  int KernelWidth;
  int KernelType;
  CartesianExtent KernelExt;
  float *Kernel;
  int KernelModified;
  //
  int Mode;
  //
  int NumberOfIterations;

private:
  vtkSQKernelConvolution(const vtkSQKernelConvolution &); // Not implemented
  void operator=(const vtkSQKernelConvolution &); // Not implemented
};

#endif
