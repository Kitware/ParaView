/*=========================================================================

  Module:    vtkKWHistogramSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWHistogramSet - a set of histograms
// .SECTION Description
// A set of histograms.

#ifndef __vtkKWHistogramSet_h
#define __vtkKWHistogramSet_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

class vtkDataArray;
class vtkKWHistogram;
class vtkKWHistogramCallback;
class vtkKWHistogramSetInternals;

class KWWIDGETS_EXPORT vtkKWHistogramSet : public vtkObject
{
public:
  static vtkKWHistogramSet* New();
  vtkTypeRevisionMacro(vtkKWHistogramSet,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an histogram to the pool under a given name. 
  // Return 1 on success, 0 otherwise.
  virtual int AddHistogram(vtkKWHistogram*, const char *name);

  // Description:
  // Allocate an histogram and add it in the pool under a given name. 
  // Return a pointer to the new histogram on success, NULL otherwise.
  virtual vtkKWHistogram* AllocateAndAddHistogram(const char *name);

  // Description:
  // Get the number of histogram in the pool
  virtual int GetNumberOfHistograms();

  // Description:
  // Retrieve an histogram (or its name) from the pool.
  virtual vtkKWHistogram* GetHistogramWithName(const char *name);
  virtual const char* GetHistogramName(vtkKWHistogram *hist);
  virtual vtkKWHistogram* GetNthHistogram(int index);

  // Description:
  // Query if the pool has a given histogram
  virtual int HasHistogramWithName(const char *name);
  virtual int HasHistogram(vtkKWHistogram *hist);

  // Description:
  // Remove one or all histograms. 
  // Return 1 on success, 0 otherwise.
  virtual int RemoveHistogramWithName(const char *name);
  virtual int RemoveHistogram(vtkKWHistogram *hist);
  virtual void RemoveAllHistograms();

  // Description:
  // The histogram set class is designed to share histogram between several
  // classes in an application. As such, since histogram are retrieved by
  // names, it makes sense to follow some naming guidelines. This method
  // provides such a guideline by computing an histogram name given the 
  // name of the array this histogram will be built upon, the component
  // that will be used in that array, and an optional tag.
  // The histogram name is stored in 'buffer', which should be large enough.
  // Return 1 on success, 0 otherwise.
  static int ComputeHistogramName(
    const char *array_name, int comp, const char *tag, char *buffer);

  // Description:
  // Allocate, add and build histograms for all components of a scalar array.
  // Each histogram name is built by calling ComputeHistogramName with
  // the scalar array, component index and 'tag' arguments.
  // The 'skip_components_mask' is a binary mask specifying which component
  // should be skipped (i.e., if the n-th bit in that mask is set, then the
  // histogram for that n-th component will not be considered)
  // Return 1 on success, 0 otherwise.
  virtual int AddHistograms(vtkDataArray *array, 
                            const char *tag = NULL, 
                            int skip_components_mask = 0);

protected:
  vtkKWHistogramSet();
  ~vtkKWHistogramSet();

  //BTX
  // PIMPL Encapsulation for STL containers
  vtkKWHistogramSetInternals *Internals;
  //ETX

private:
  vtkKWHistogramSet(const vtkKWHistogramSet&); // Not implemented
  void operator=(const vtkKWHistogramSet&); // Not implemented
};

#endif

