/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeRenderManager.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/
// .NAME vtkCompositeRenderManager - An object to control sort-last parallel rendering.
//
// .SECTION Description:
// One day I hope this class replaces vtkCompositeManager.
//
// .SECTION Note:
// This class does not handle transparent objects.
//

#ifndef __vtkCompositeRenderManager_h
#define __vtkCompositeRenderManager_h

#include "vtkParallelRenderManager.h"

class vtkCompositer;

class VTK_EXPORT vtkCompositeRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeRevisionMacro(vtkCompositeRenderManager, vtkParallelRenderManager);
  static vtkCompositeRenderManager *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/Get the composite algorithm.
  void SetCompositer(vtkCompositer *c);
  vtkGetObjectMacro(Compositer, vtkCompositer);

  // Description:
  // Get rendering metrics.
  vtkGetMacro(ImageProcessingTime, double);

protected:
  vtkCompositeRenderManager();
  ~vtkCompositeRenderManager();

  vtkCompositer *Compositer;

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  vtkFloatArray *DepthData;
  vtkUnsignedCharArray *TmpPixelData;
  vtkFloatArray *TmpDepthData;

private:
  vtkCompositeRenderManager(const vtkCompositeRenderManager &);//Not implemented
  void operator=(const vtkCompositeRenderManager &);    //Not implemented
};

#endif //__vtkCompositeRenderManager_h
