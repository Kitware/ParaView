/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPVoronoiReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPVoronoiReader.h -- Reads a voronoi tesselation file.
//
// .SECTION Description
//  Reads in a voronoi tesselation, produced by TESS (a voronoi tesselation
//  code for cosmological datasets), and stores it in a multi-block dataset
//  of unstructured grids.

#ifndef __VTKPVORONOIREADER_h
#define __VTKPVORONOIREADER_h

#include <vtkMultiBlockDataSetAlgorithm.h>
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

#include <vtkSmartPointer.h>
#include "voronoi.h"

class vtkMultiProcessController;
class vtkUnstructuredGrid;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPVoronoiReader :
  public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPVoronoiReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPVoronoiReader *New();

  // Description:
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Allow user to set the header size
  vtkSetMacro(HeaderSize,int);

protected:
  vtkPVoronoiReader();
  ~vtkPVoronoiReader();

  int FillOutputPortInformation( int port, vtkInformation* info );
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *);

private:
  vtkPVoronoiReader(const vtkPVoronoiReader&);  // Not implemented.
  void operator=(const vtkPVoronoiReader&);  // Not implemented.

  //ser_io
  void ReadFooter(FILE*& fd, int64_t*& ftr, int& tb);
  void ReadHeader(FILE *fd, int *hdr, int64_t ofst);
  int CopyHeader(unsigned char *in_buf, int *hdr);
  void ReadBlock(FILE *fd, vblock_t* &v, int64_t ofst);
  void CopyBlock(unsigned char *in_buf, vblock_t* &v);

  int dim; // number of dimensions in the dataset
  bool swap_bytes; // whether to swap bytes for endian conversion

  int HeaderSize;

  //swap
  void Swap(char *n, int nitems, int item_size);
  void Swap8(char *n);
  void Swap4(char *n);
  void Swap2(char *n);

  //parallelism
  int NumProcesses;
  int MyId;
  vtkMultiProcessController *Controller;
  void SetController(vtkMultiProcessController *c);

  //voronoi
  void vor2ugrid(struct vblock_t *block, vtkSmartPointer<vtkUnstructuredGrid> &ugrid);

  char* FileName;
};

#endif
