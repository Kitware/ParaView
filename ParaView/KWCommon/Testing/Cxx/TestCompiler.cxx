#include "vtkObject.h"

int TestConst(char*){ return 1; }
int TestConst(const char*) { return 0; }

int main(int, char**)
{
  char* s = new char[ 10 ];
  int res = TestConst(s);
  delete [] s;
  cout << "Result: " << res << endl;
  res = TestConst("andy");
  cout << "Result: " << res << endl;
  return res;
}

