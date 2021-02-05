#ifndef CDI_READER_HELPERS_
#define CDI_READER_HELPERS_

#include <string>

std::string GetPathName(const std::string& s);
std::string GetBasenameFromUri(const std::string& s);
bool CheckFileAccess(const std::string& name);

void Strip(std::string& s);

std::string ConvertInt(int number);

#endif /* CDI_READER_HELPERS_ */
