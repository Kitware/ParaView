#include "vtkPiecewiseFunction.h"
#include "vtkProperty.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLPiecewiseFunctionWriter.h"
#include "vtkXMLPropertyReader.h"
#include "vtkXMLPropertyWriter.h"
#include "vtkXMLUtilities.h"
#include "vtkXMLVolumePropertyWriter.h"

//----------------------------------------------------------------------------
int TestWriter()
{
  cout << "TestWriter..." << endl;

  vtkProperty *prop = vtkProperty::New();
  prop->SetAmbient(0.5);
  
  vtkXMLPropertyWriter *propw = vtkXMLPropertyWriter::New();
  propw->SetObject(prop);
  propw->SetWriteFactored(0);
  propw->SetWriteIndented(1);
  int res = propw->WriteToFile("testxmlrw.xml");
  if (!res)
    {
    cerr << "TestWriter... error !" << endl;
    }

  propw->Delete();
  prop->Delete();

  return res;
}

//----------------------------------------------------------------------------
int TestCreate()
{
  cout << "TestCreate..." << endl;

  vtkXMLDataElement *elem = vtkXMLDataElement::New();
  elem->SetName("MyRoot");

  vtkProperty *prop = vtkProperty::New();
  prop->SetAmbient(0.5);
  
  vtkXMLPropertyWriter *propw = vtkXMLPropertyWriter::New();
  propw->SetObject(prop);
  int res = propw->CreateInElement(elem);
  if (!res)
    {
    cerr << "TestCreate... error !" << endl;
    }

  vtkIndent indent;
  vtkXMLUtilities::FlattenElement(elem, cout, &indent, 1);

  propw->Delete();
  prop->Delete();
  elem->Delete();

  return res;
}

//----------------------------------------------------------------------------
int TestReader()
{
  cout << "TestReader..." << endl;

  vtkProperty *prop = vtkProperty::New();
  
  vtkXMLPropertyReader *propr = vtkXMLPropertyReader::New();
  propr->SetObject(prop);
  int res = propr->ParseFile("testxmlrw.xml");
  if (!res)
    {
    cerr << "TestReader... error !" << endl;
    }

  prop->PrintSelf(cout, vtkIndent());

  propr->Delete();
  prop->Delete();

  return res;
}

//----------------------------------------------------------------------------
int TestParse()
{
  cout << "TestParse..." << endl;

  vtkProperty *prop = vtkProperty::New();
  
  vtkXMLPropertyReader *propr = vtkXMLPropertyReader::New();
  propr->SetObject(prop);
  int res = propr->ParseString("<Property Ambient='0.5'/>");
  if (!res)
    {
    cerr << "TestParse... error !" << endl;
    }

  prop->PrintSelf(cout, vtkIndent());

  propr->Delete();
  prop->Delete();

  return res;
}

//----------------------------------------------------------------------------
int TestComplex()
{
  cout << "TestComplex..." << endl;

  vtkPiecewiseFunction *pf = vtkPiecewiseFunction::New();
  pf->AddPoint(0.3, 128.0);
  pf->AddPoint(0.5, 255.0);

  vtkXMLPiecewiseFunctionWriter *pfw = vtkXMLPiecewiseFunctionWriter::New();
  pfw->SetObject(pf);
  pfw->SetWriteFactored(0);
  pfw->SetWriteIndented(1);
  int res = pfw->WriteToFile("testxmlrw_pf.xml");
  if (!res)
    {
    cerr << "TestComplex... error !" << endl;
    }

  pfw->Delete();
  pf->Delete();

  vtkVolumeProperty *vp = vtkVolumeProperty::New();
  
  vtkXMLVolumePropertyWriter *vpw = vtkXMLVolumePropertyWriter::New();
  vpw->SetObject(vp);
  vpw->SetWriteFactored(0);
  vpw->SetWriteIndented(1);
  res = vpw->WriteToFile("testxmlrw_vp.xml");
  if (!res)
    {
    cerr << "TestComplex... error !" << endl;
    }

  vpw->Delete();
  vp->Delete();

  return res;
}

//----------------------------------------------------------------------------
void display_usage(int, char **argv)
{
  cout << argv[0] << " -w -c -r -p -x" << endl;
}

//----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  display_usage(argc, argv);

  int i, res = 0;
  for (i = 1; i < argc; i++)
    {
    if (argv[i] && argv[i][0] == '-')
      {
      switch (argv[i][1])
        {
        case 'w':
          res += TestWriter();
          break;
        case 'c':
          res += TestCreate();
          break;
        case 'r':
          res += TestReader();
          break;
        case 'p':
          res += TestParse();
          break;
        case 'x':
          res += TestComplex();
          break;
        }
      }
    }

  return res;
}
