/*=========================================================================

  Program:   ParaView
  Module:    vtkConduitSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkConduitSource
 * @brief data source for Conduit Mesh Blueprint.
 * @ingroup Insitu
 *
 * vtkConduitSource is a data source that processes a Conduit node
 * using [Conduit Mesh
 * Blueprint](https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#)
 * to describe computational mesh and associated meta-data.
 *
 * vtkConduitSource currently produces a `vtkParitionedDataSet`. This makes it
 * easier to support mesh definitions with sub-domains.
 *
 * @sa vtkConduitArrayUtilities
 */

#ifndef vtkConduitSource_h
#define vtkConduitSource_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsConduitModule.h" // for exports
#include <memory>                            // for std::unique_ptr
extern "C" {
typedef void conduit_node;
}
class VTKPVVTKEXTENSIONSCONDUIT_EXPORT vtkConduitSource : public vtkDataObjectAlgorithm
{
public:
  static vtkConduitSource* New();
  vtkTypeMacro(vtkConduitSource, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the conduit_node. This must be the node satisfying the Conduit Mesh
   * Blueprint.
   */
  void SetNode(const conduit_node* node);
  //@}

protected:
  vtkConduitSource();
  ~vtkConduitSource();

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkConduitSource(const vtkConduitSource&) = delete;
  void operator=(const vtkConduitSource&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
