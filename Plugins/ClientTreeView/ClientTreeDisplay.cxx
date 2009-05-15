/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "ClientTreeDisplay.h"
#include "ui_ClientTreeDisplay.h"

#include <vtkTree.h>

class ClientTreeDisplay::implementation
{
public:
  implementation()
  {
  }

  ~implementation()
  {
  }

};

ClientTreeDisplay::ClientTreeDisplay(pqRepresentation* representation, QWidget* p) :
  pqDisplayPanel(representation, p),
  Implementation(new implementation())
{
  // I think if the view has properties and such you set them up here
}

ClientTreeDisplay::~ClientTreeDisplay()
{
  delete this->Implementation;
}
