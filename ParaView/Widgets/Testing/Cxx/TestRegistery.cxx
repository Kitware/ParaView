#include "vtkKWRegisteryUtilities.h"

#define IFT(x,res) if ( !x ) \
  { \
  res = 1; \
  cout << "Error in: " << #x << endl; \
  }
#define IFNT(x,res) if ( x ) \
  { \
  res = 1; \
  cout << "Error in: " << #x << endl; \
  }

#define CHE(x,y,res) if ( strcmp(x,y) ) \
  { \
  res = 1; \
  cout << "Error, " << x << " != " << y << endl;\
  }

int main(int vtkNotUsed(argc), char* vtkNotUsed(argv))
{
  int res = 0;
  
  vtkKWRegisteryUtilities *reg = vtkKWRegisteryUtilities::New();
  reg->SetTopLevel("TestRegistry");
  
  IFT(reg->SetValue("TestSubkey",  "TestKey1", "Test Value 1"), res);
  IFT(reg->SetValue("TestSubkey1", "TestKey2", "Test Value 2"), res);
  IFT(reg->SetValue("TestSubkey",  "TestKey3", "Test Value 3"), res);
  IFT(reg->SetValue("TestSubkey2", "TestKey4", "Test Value 4"), res);

  char buffer[1024];
  IFT(reg->ReadValue("TestSubkey",  "TestKey1", buffer), res);
  CHE(buffer, "Test Value 1", res);
  IFT(reg->ReadValue("TestSubkey1", "TestKey2", buffer), res);
  CHE(buffer, "Test Value 2", res);
  IFT(reg->ReadValue("TestSubkey",  "TestKey3", buffer), res);
  CHE(buffer, "Test Value 3", res);
  IFT(reg->ReadValue("TestSubkey2", "TestKey4", buffer), res);
  CHE(buffer, "Test Value 4", res);
 
  IFT(reg->SetValue("TestSubkey",  "TestKey1", "New Test Value 1"), res);
  IFT(reg->SetValue("TestSubkey1", "TestKey2", "New Test Value 2"), res);
  IFT(reg->SetValue("TestSubkey",  "TestKey3", "New Test Value 3"), res);
  IFT(reg->SetValue("TestSubkey2", "TestKey4", "New Test Value 4"), res);

  IFT(reg->ReadValue("TestSubkey",  "TestKey1", buffer), res);
  CHE(buffer, "New Test Value 1", res);
  IFT(reg->ReadValue("TestSubkey1", "TestKey2", buffer), res);
  CHE(buffer, "New Test Value 2", res);
  IFT(reg->ReadValue("TestSubkey",  "TestKey3", buffer), res);
  CHE(buffer, "New Test Value 3", res);
  IFT(reg->ReadValue("TestSubkey2", "TestKey4", buffer), res);
  CHE(buffer, "New Test Value 4", res);

  IFT( reg->DeleteValue("TestSubkey",  "TestKey1"), res);
  IFNT(reg->ReadValue(  "TestSubkey",  "TestKey1", buffer), res);
  IFT( reg->DeleteValue("TestSubkey1", "TestKey2"), res);
  IFNT(reg->ReadValue(  "TestSubkey1", "TestKey2", buffer), res);
  IFT( reg->DeleteValue("TestSubkey",  "TestKey3"), res);
  IFNT(reg->ReadValue(  "TestSubkey",  "TestKey3", buffer), res);
  IFT( reg->DeleteValue("TestSubkey2", "TestKey4"), res);
  IFNT(reg->ReadValue(  "TestSubkey2", "TestKey5", buffer), res);  

  reg->Delete();
  if ( res )
    {
    cout << "Test failed" << endl;
    }
  return res;
}
