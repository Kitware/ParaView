#include "vtkObject.h"
#include "vtkString.h"
#include "vtkObjectFactory.h"

class TestClass : public vtkObject 
{
public:
  vtkTypeMacro(TestClass, vtkObject);
  
  static TestClass* New();
  
  vtkSetMacro(Result, int);
  vtkGetMacro(Result, int);
  
  int TestConst(char*);
  int TestConst(const char*);
  
private:
  TestClass() { this->Result = 0; }

  int Result;
  TestClass(const TestClass&); // Not implement
  void operator=(const TestClass&); // Not implement
};

vtkStandardNewMacro(TestClass);

int TestClass::TestConst(char* s)
{
  int cc;
  int length = vtkString::Length(s);
  for ( cc =0; cc < (length/2)+1; cc ++ )
    {
    char tmp = s[length - cc - 1];
    s[length - cc - 1] = s[cc];
    s[cc] = tmp;
    }
  cout << "String: " << s << endl;
  this->Result = 1;
  return 1;
}

int TestClass::TestConst(const char* s)
{
  int cc;
  char * copy = vtkString::Duplicate(s);
  int length = vtkString::Length(copy);
  for ( cc =0; cc < (length/2)+1; cc ++ )
    {
    char tmp = copy[length - cc - 1];
    copy[length - cc - 1] = copy[cc];
    copy[cc] = tmp;
    }
  cout << "ConstString: " << copy << endl;
  delete [] copy;
  this->Result = 0;
  return 0;
}

int main(int, char** argv)
{
  TestClass* test = TestClass::New();
  test->TestConst(argv[0]);
  int res = test->GetResult();
  test->Delete();
  return res;
}
