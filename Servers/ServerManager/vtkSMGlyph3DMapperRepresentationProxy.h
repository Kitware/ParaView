/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlyph3DMapperRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMGlyph3DMapperRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMGlyph3DMapperRepresentationProxy_h
#define __vtkSMGlyph3DMapperRepresentationProxy_h

#include "vtkSMSurfaceRepresentationProxy.h"
#include "vtkSmartPointer.h" //needed for maintaining the protected Source Proxy

class VTK_EXPORT vtkSMGlyph3DMapperRepresentationProxy : public vtkSMSurfaceRepresentationProxy
{
public:
  static vtkSMGlyph3DMapperRepresentationProxy* New();
  vtkTypeMacro(vtkSMGlyph3DMapperRepresentationProxy, vtkSMSurfaceRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkSMInputProperty requires that the consumer proxy support
  // AddInput() method. Hence, this method is defined. This method
  // sets up the input connection.
  // Overridden to handle the glyph source input connection.
  virtual void AddInput(unsigned int inputPort, vtkSMSourceProxy* input,
    unsigned int outputPort, const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
     { this->Superclass::AddInput(input, method); }

  // Description:
  // Get the bounds and transform according to rotation, translation, and scaling.
  // Returns true if the bounds are "valid" (and false otherwise)
  virtual bool GetBounds(double bounds[6]);

//BTX
protected:
  vtkSMGlyph3DMapperRepresentationProxy();
  ~vtkSMGlyph3DMapperRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // If this method returns false, CreateVTKObjects() is aborted.
  // Overridden to abort CreateVTKObjects() only if the input has
  // been initialized correctly.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects().
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  // Overridden to setup view time link.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Some representations may require lod/compositing strategies from the view
  // proxy. This method gives such subclasses an opportunity to as the view
  // module for the right kind of strategy and plug it in the representation
  // pipeline. Returns true on success. Default implementation suffices for
  // representation that don't use strategies.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

  vtkSmartPointer<vtkSMProxy> Source;
  unsigned int SourceOutputPort;
  vtkSMProxy* GlyphMapper;
  vtkSMProxy* LODGlyphMapper;

  vtkSMRepresentationStrategy* GlyphSourceStrategy;
private:
  vtkSMGlyph3DMapperRepresentationProxy(const vtkSMGlyph3DMapperRepresentationProxy&); // Not implemented
  void operator=(const vtkSMGlyph3DMapperRepresentationProxy&); // Not implemented
//ETX
};

#endif

