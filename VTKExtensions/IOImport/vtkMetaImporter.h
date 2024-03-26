// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMetaImporter
 * @brief   An interface to access VTK Importers from ParaView server manager.
 *
 * This class creates the correct vtkImporter subclass based on the file extension.
 * It provides common metadata like scene hierarchy and a description of the scene.
 * The metadata is accessible after calling UpdatePipelineInformation.
 */

#ifndef vtkMetaImporter_h
#define vtkMetaImporter_h

#include "vtkObject.h"

#include "vtkAOSDataArrayTemplate.h"          // for PayloadType
#include "vtkPVVTKExtensionsIOImportModule.h" // for exports
#include "vtkVariant.h"                       // for return

#include <memory> // for unique_ptr

class vtkDataAssembly;
class vtkDataObject;
class vtkImporter;
class vtkStringArray;

class VTKPVVTKEXTENSIONSIOIMPORT_EXPORT vtkMetaImporter : public vtkObject
{
public:
  static vtkMetaImporter* New();
  vtkTypeMacro(vtkMetaImporter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the file name.
   */
  void SetFileName(const char* filename);
  const char* GetFileName();
  ///@}

  /**
   * Imports the scene and populates scene hierarchy as well as scene description.
   */
  void UpdateInformation();

  ///@{
  /**
   * The scene description i.e, material, texture, transformation matrices for actors,
   * cameras and lights is serialized to JSON and transformed into
   * CBOR (Consise Binary Object Representation) stored in a vtkTypeUInt8Array for easier
   * transmission across the network.
   */
  using PayloadType = vtkAOSDataArrayTemplate<vtkTypeUInt8>;
  vtkVariant GetSerializedSceneDescription();
  ///@}

  ///@{
  /**
   * The hierarchy of actors in the renderer is encoded in a vtkDataAssembly.
   */
  vtkDataAssembly* GetAssembly();
  ///@}

  ///@{
  /**
   * Get/Set the name of the assembly to use for mapping block visibilities,
   * colors and opacities.
   *
   * The default is Hierarchy.
   */
  vtkSetStringMacro(ActiveAssembly);
  vtkGetStringMacro(ActiveAssembly);
  ///@}

  /**
   * Whenever the assembly is changed, this tag gets changed. Note, users should
   * not assume that this is monotonically increasing but instead simply rely on
   * its value to determine if the assembly may have changed since last time.
   *
   * It is set to 0 whenever there's no valid assembly available.
   */
  vtkGetMacro(AssemblyTag, int);

  ///@{
  /**
   * Update list of selectors that determine the selected nodes.
   */
  void AddNodeSelector(const char* selector);
  void RemoveAllNodeSelectors();
  ///@}

protected:
  vtkMetaImporter();
  ~vtkMetaImporter() override;

  int AssemblyTag = 0;
  char* ActiveAssembly = nullptr;
  std::string FileName;

private:
  vtkMetaImporter(const vtkMetaImporter&) = delete;
  void operator=(const vtkMetaImporter&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
