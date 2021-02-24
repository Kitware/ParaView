/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Provide support for loading imago images in VR
 */

// de = "SI-17-71"; // collection -- need collectionid
// ds = "Downhole"; // imagery type -- need imagery type id -- in worspaces, multiple definitions
// im = "Wet";      // image type -- in workspace -- multiple definitions
// dp = "0";        // depth -- start depth or enddepth
// da = dataset name

#include "vtkImageClip.h"
#include "vtkImagePermute.h"
#include <future>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <vtk_jsoncpp.h>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "Wininet.lib")

class vtkImagoLoader
{
public:
  // get an image based on an imago URL
  // parses args and then calls the long signature
  bool GetHttpImage(std::string const& fname, std::future<vtkImageData*>& future);

  // get an image based on an holeid and depth
  // parses args and then calls the long signature
  bool GetImage(std::string const& fname, std::future<vtkImageData*>& future);

  // the long signature that does the work
  bool GetImage(std::string const& workspace, std::string const& dataset,
    std::string const& collection, std::string const& imageryType, std::string const& imageType,
    std::string const& depth, std::future<vtkImageData*>& future);

  bool IsCellImageDifferent(std::string const& /*oldimg*/, std::string const& newimg);

  bool Connect();
  bool AcquireToken();
  bool GetContext();

  bool Login(std::string const& uname, std::string const& pw)
  {
    this->UserName = uname;
    this->Password = pw;
    if (!this->Connect() || !this->AcquireToken() || !this->GetContext())
    {
      return false;
    }
    return true;
  }
  bool IsLoggedIn() { return this->ContextJSON.size() > 0; }

  void SetWorkspace(std::string const& val) { this->Workspace = val; }
  void SetDataset(std::string const& val) { this->Dataset = val; }
  void SetImageryType(std::string const& val) { this->ImageryType = val; }
  void SetImageType(std::string const& val) { this->ImageType = val; }

  void GetWorkspaces(std::vector<std::string>& vals);
  void GetDatasets(std::vector<std::string>& vals);
  void GetImageryTypes(std::vector<std::string>& vals);
  void GetImageTypes(std::vector<std::string>& vals);

  vtkImagoLoader();
  virtual ~vtkImagoLoader();

protected:
  bool GetCollection(
    std::string const& collection, std::string const& dataset, Json::Value& resultJSON);
  bool GetImagery(
    std::string const& collection, std::string const& dataset, Json::Value& resultJSON);

  HINTERNET Session = nullptr;
  HINTERNET Connection = nullptr;
  std::string UID;
  std::string APIToken;
  Json::Value ContextJSON;
  std::string UserName;
  std::string Password;
  std::string Workspace;
  std::string Dataset;
  std::string ImageryType;
  std::string ImageType;
  std::atomic<double> LastStartDepth;
  std::atomic<double> LastEndDepth;
  std::list<std::pair<std::string, vtkSmartPointer<vtkImageData> > > ImageCache;
  unsigned int CacheLimit = 20;
  std::map<std::pair<std::string, std::string>, Json::Value> CollectionMap;
  std::map<std::pair<std::string, std::string>, Json::Value> ImageryMap;
  std::mutex Mutex;
};

namespace
{

bool escapeURL(std::string& url, DWORD options = ICU_DECODE | ICU_ENCODE_PERCENT)
{
  DWORD bytes = static_cast<DWORD>(url.size() + 1);
  LPTSTR escapedString = new TCHAR[bytes];
  escapedString[0] = 0;

  // First read will generate the correct byte count for the return string
  //
  bool result = InternetCanonicalizeUrl(url.data(), escapedString, &bytes, options);
  if (!result)
  {
    // Resize the String
    delete[] escapedString;
    escapedString = new TCHAR[bytes];
    escapedString[0] = 0;

    result = InternetCanonicalizeUrl(url.data(), escapedString, &bytes, options);
  }

  if (result)
  {
    url = escapedString;
    delete[] escapedString;
  }

  return result;
}

bool getResult(HINTERNET hRequest, std::vector<char>& result)
{
  static const int bufSize = 10000;
  result.clear();
  std::vector<char> buffer;
  buffer.resize(bufSize);

  while (true)
  {
    DWORD bytesRead;
    bool success;

    success = InternetReadFile(hRequest, buffer.data(), bufSize, &bytesRead);

    // only complete when bytesRead == 0
    if (success && bytesRead == 0)
    {
      break;
    }

    if (!success)
    {
      vtkGenericWarningMacro("InternetReadFile error : " << GetLastError());
      return false;
    }

    if (bytesRead > 0)
    {
      buffer.resize(bytesRead);
      result.insert(result.end(), buffer.begin(), buffer.end());
    }
  }

  return true;
}

bool sendMessage(HINTERNET connection, std::string type, std::string address,
  std::vector<std::string> const& headers, std::string formData, std::vector<char>& result)
{
  PCTSTR rgpszAcceptTypes[] = { "*/*", nullptr };
  HINTERNET hRequest = HttpOpenRequest(connection, (LPCSTR)(type.c_str()),
    (LPCSTR)(address.c_str()), nullptr, nullptr, rgpszAcceptTypes, INTERNET_FLAG_RELOAD |
      INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NEED_FILE | INTERNET_FLAG_SECURE,
    0);
  if (!hRequest)
  {
    vtkGenericWarningMacro("HttpOpenRequest error : " << GetLastError());
    return false;
  }

  for (auto& hdr : headers)
  {
    HttpAddRequestHeaders(hRequest, hdr.c_str(), static_cast<DWORD>(-1),
      HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
  }

  // clang-format off
  std::string frmdata = R"({ "username": "imagodemo1", "password": "ImagoDemo1" })";
  // clang-format on

  while (!HttpSendRequest(
    hRequest, nullptr, 0, (LPVOID)formData.c_str(), static_cast<DWORD>(formData.size())))
  {
    InternetErrorDlg(GetDesktopWindow(), hRequest, ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
      FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
        FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
      nullptr);
  }

  if (!getResult(hRequest, result))
  {
    return false;
  }

  InternetCloseHandle(hRequest);
  return true;
}

Json::Value getJSON(std::vector<char> const& result)
{
  Json::Value root;
  {
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    bool success = reader->parse(result.data(), result.data() + result.size(), &root, nullptr);
    if (!success)
    {
      return false;
    }
  }
  return root;
}
}

vtkImagoLoader::vtkImagoLoader()
{
}

vtkImagoLoader::~vtkImagoLoader()
{
  if (this->Connection)
  {
    InternetCloseHandle(this->Connection);
    this->Connection = nullptr;
  }
  if (this->Session)
  {
    InternetCloseHandle(this->Session);
    this->Session = nullptr;
  }
}

bool vtkImagoLoader::Connect()
{
  if (!this->Session)
  {
    this->Session = InternetOpen("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!this->Session)
    {
      vtkGenericWarningMacro("InternetOpen error : " << GetLastError());
      return false;
    }
  }

  if (!this->Connection)
  {
    this->Connection = InternetConnect(this->Session, "io.imago.live", INTERNET_DEFAULT_HTTPS_PORT,
      "", "", INTERNET_SERVICE_HTTP, 0, 0);
    if (!this->Connection)
    {
      vtkGenericWarningMacro("InternetConnect error : " << GetLastError());
      return false;
    }
  }

  return true;
}

void vtkImagoLoader::GetWorkspaces(std::vector<std::string>& vals)
{
  vals.clear();
  if (!this->Connect() || !this->AcquireToken() || !this->GetContext())
  {
    return;
  }

  for (auto& w : this->ContextJSON["workspaces"])
  {
    std::string itname = w["name"].asString();
    if (std::find(vals.begin(), vals.end(), itname) == vals.end())
    {
      vals.push_back(itname);
    }
  }
}

void vtkImagoLoader::GetDatasets(std::vector<std::string>& vals)
{
  vals.clear();
  if (!this->Connect() || !this->AcquireToken() || !this->GetContext())
  {
    return;
  }

  for (auto& w : this->ContextJSON["workspaces"])
  {
    // is this the correct workspace
    if (this->Workspace.size() == 0 || this->Workspace == w["name"].asString())
    {
      auto& ds = w["datasets"];
      for (auto& d : ds)
      {
        std::string itname = d["name"].asString();
        if (std::find(vals.begin(), vals.end(), itname) == vals.end())
        {
          vals.push_back(itname);
        }
      }
    }
  }
}

void vtkImagoLoader::GetImageryTypes(std::vector<std::string>& vals)
{
  vals.clear();
  if (!this->Connect() || !this->AcquireToken() || !this->GetContext())
  {
    return;
  }

  for (auto& w : this->ContextJSON["workspaces"])
  {
    // is this the correct workspace
    if (this->Workspace.size() == 0 || this->Workspace == w["name"].asString())
    {
      auto& ds = w["datasets"];
      for (auto& d : ds)
      {
        // is this the correct dataset?
        if (this->Dataset.size() == 0 || this->Dataset == d["name"].asString())
        {
          auto& its = d["imageryTypes"];
          for (auto& it : its)
          {
            std::string itname = it["name"].asString();
            if (std::find(vals.begin(), vals.end(), itname) == vals.end())
            {
              vals.push_back(itname);
            }
          }
        }
      }
    }
  }
}

void vtkImagoLoader::GetImageTypes(std::vector<std::string>& vals)
{
  vals.clear();
  if (!this->Connect() || !this->AcquireToken() || !this->GetContext())
  {
    return;
  }

  for (auto& w : this->ContextJSON["workspaces"])
  {
    // is this the correct workspace
    if (this->Workspace.size() == 0 || this->Workspace == w["name"].asString())
    {
      auto& ds = w["datasets"];
      for (auto& d : ds)
      {
        // is this the correct dataset?
        if (this->Dataset.size() == 0 || this->Dataset == d["name"].asString())
        {
          auto& its = d["imageryTypes"];
          for (auto& it : its)
          {
            auto& imageTypes = it["imageTypes"];
            for (auto& imageType : imageTypes)
            {
              std::string itname = imageType["name"].asString();
              if (std::find(vals.begin(), vals.end(), itname) == vals.end())
              {
                vals.push_back(itname);
              }
            }
          }
        }
      }
    }
  }
}

bool vtkImagoLoader::IsCellImageDifferent(std::string const& /*oldimg*/, std::string const& newimg)
{
  std::smatch sm;

  // look for http signature
  if (!strncmp(newimg.c_str(), "http", 4))
  {
    // clang-format off
    std::regex matcher(R"=([^ ]*\?((?:de|ds|im|dp)=[^&]*)(&(?:de|ds|im|dp)=[^&]*)?(&(?:de|ds|im|dp)=[^&]*)?(&(?:de|ds|im|dp)=[^&]*)?)=");
    // clang-format on
    std::regex_search(newimg, sm, matcher);
  }
  // now check holieid signature
  else if (!strncmp(newimg.c_str(), "holeid:", 7))
  {
    // clang-format off
    std::regex matcher(R"=(holeid:(de=[^&]*)(&dp=[^&]*))=");
    // clang-format on
    std::regex_search(newimg, sm, matcher);
  }
  else
  {
    return true;
  }

  std::map<std::string, std::string> args;
  for (size_t i = 1; i < sm.size(); ++i)
  {
    std::string first = sm[i];
    if (first[0] == '&')
    {
      first = first.substr(1);
    }
    args[first.substr(0, 2)] = first.substr(3);
  }

  // if we are missing key arguments then warn and return
  if (args.find("dp") == args.end())
  {
    return true;
  }
  double depth = std::stod(args["dp"].c_str());

  // is this depth a new image?
  if (depth >= this->LastStartDepth && depth < this->LastEndDepth)
  {
    return false;
  }
  return true;
}

bool vtkImagoLoader::AcquireToken()
{
  std::vector<char> result;

  if (this->APIToken.size())
  {
    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json\r\n");
    headers.push_back("imago-api-token: " + this->APIToken + "\r\n");
    if (!sendMessage(this->Connection, "GET", "/integrate/2/session", headers, "", result))
    {
      vtkGenericWarningMacro("Failed to open session");
      return false;
    }

    Json::Value root = getJSON(result);
    if (!root.isObject() || !root.isMember("uid"))
    {
      this->APIToken.clear();
    }
  }

  if (!this->APIToken.size())
  {
    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json\r\n");
    std::string tokenBody =
      "{ \"username\": \"" + this->UserName + "\", \"password\": \"" + this->Password + "\" }";
    if (!sendMessage(this->Connection, "POST", "/integrate/2/session", headers, tokenBody, result))
    {
      vtkGenericWarningMacro("Failed to open session");
      return false;
    }

    Json::Value root = getJSON(result);
    if (!root.isObject() || !root.isMember("uid") || !root.isMember("apiToken"))
    {
      return false;
    }
    this->UID = root["uid"].asString();
    this->APIToken = root["apiToken"].asString();
  }

  return true;
}

bool vtkImagoLoader::GetContext()
{
  // return if we already loaded it
  if (!this->ContextJSON.empty())
  {
    return true;
  }

  std::vector<char> result;
  std::vector<std::string> headers;

  // find matching collection to get collectionid etc
  std::string frmdata;
  // get the context and then extract ids from it
  headers.push_back("Content-Type: application/x-www-form-urlencoded\r\n");
  headers.push_back("imago-api-token: " + this->APIToken + "\r\n");
  if (!sendMessage(this->Connection, "GET", "/integrate/2/context", headers, frmdata, result))
  {
    return false;
  }
  this->ContextJSON = getJSON(result);

  if (!this->ContextJSON.isObject() || !this->ContextJSON.isMember("workspaces"))
  {
    this->ContextJSON.clear();
    vtkGenericWarningMacro("Failed to get context from Imago");
    return false;
  }

  return true;
}

bool vtkImagoLoader::GetImage(std::string const& fname, std::future<vtkImageData*>& future)
{
  std::smatch sm;
  // clang-format off
  std::regex matcher(R"=(holeid:(de=[^&]*)(&dp=[^&]*))=");
  // clang-format on
  std::regex_search(fname, sm, matcher);

  std::map<std::string, std::string> args;
  for (size_t i = 1; i < sm.size(); ++i)
  {
    std::string first = sm[i];
    if (first[0] == '&')
    {
      first = first.substr(1);
    }
    args[first.substr(0, 2)] = first.substr(3);
  }

  // if we are missing key arguments then warn and return
  if (args.find("de") == args.end())
  {
    vtkGenericWarningMacro("Lacking collection name for image");
    return false;
  }

  return this->GetImage(this->Workspace, this->Dataset != "Any" ? this->Dataset : "", args["de"],
    this->ImageryType != "Any" ? this->ImageryType : "",
    this->ImageType != "Any" ? this->ImageType : "", args["dp"], future);
}

bool vtkImagoLoader::GetHttpImage(std::string const& fname, std::future<vtkImageData*>& future)
{
  std::smatch sm;
  // clang-format off
  std::regex matcher(R"=([^ ]*\?((?:de|ds|im|dp)=[^&]*)(&(?:de|ds|im|dp)=[^&]*)?(&(?:de|ds|im|dp)=[^&]*)?(&(?:de|ds|im|dp)=[^&]*)?)=");
  // clang-format on
  std::regex_search(fname, sm, matcher);

  std::map<std::string, std::string> args;
  for (size_t i = 1; i < sm.size(); ++i)
  {
    std::string first = sm[i];
    if (first[0] == '&')
    {
      first = first.substr(1);
    }
    args[first.substr(0, 2)] = first.substr(3);
  }

  // if we are missing key arguments then warn and return
  if (args.find("de") == args.end())
  {
    vtkGenericWarningMacro("Lacking collection name for image");
    return false;
  }

  return this->GetImage(this->Workspace, this->Dataset != "Any" ? this->Dataset : "", args["de"],
    args["ds"], args["im"], args["dp"], future);
}

bool vtkImagoLoader::GetCollection(
  std::string const& collection, std::string const& datasetID, Json::Value& resultJSON)
{
  decltype(this->CollectionMap)::key_type key(collection, datasetID);

  // do we already have it in the map?
  auto it = this->CollectionMap.find(key);
  if (it != this->CollectionMap.end())
  {
    resultJSON = it->second;
    return true;
  }

  if (!this->AcquireToken())
  {
    return false;
  }

  std::vector<char> result;

  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json\r\n");
  headers.push_back("imago-api-token: " + this->APIToken + "\r\n");

  // find matching collection to get collectionid etc
  std::string frmdata;
  std::string urldata = "name=" + collection;
  if (datasetID.size())
  {
    urldata += "&datasetid=" + datasetID;
  }
  // escapeURL(urldata);
  if (!sendMessage(
        this->Connection, "GET", "/integrate/2/collection?" + urldata, headers, frmdata, result))
  {
    return false;
  }
  resultJSON = getJSON(result);

  if (!resultJSON.isObject() || !resultJSON.isMember("collections") ||
    resultJSON["collections"].size() == 0)
  {
    vtkGenericWarningMacro("Failed to find matching collection for: " << collection);
    return false;
  }

  this->CollectionMap[key] = resultJSON;
  return true;
}

bool vtkImagoLoader::GetImagery(
  std::string const& collectionID, std::string const& imageryTypeID, Json::Value& resultJSON)
{
  decltype(this->ImageryMap)::key_type key(collectionID, imageryTypeID);

  // do we already have it in the map?
  auto it = this->ImageryMap.find(key);
  if (it != this->ImageryMap.end())
  {
    resultJSON = it->second;
    if (!resultJSON.isObject() || !resultJSON.isMember("imageries") ||
      resultJSON["imageries"].size() == 0)
    {
      return false;
    }
    return true;
  }

  if (!this->AcquireToken())
  {
    return false;
  }

  std::vector<char> result;

  std::vector<std::string> headers;
  headers.push_back("Content-Type: application/json\r\n");
  headers.push_back("imago-api-token: " + this->APIToken + "\r\n");

  // find matching collection to get collectionid etc
  std::string frmdata;
  // now search for the imageryID and mimeType for the image
  std::string urldata = "collectionid=" + collectionID;
  if (imageryTypeID.size())
  {
    urldata += "&imagerytypeid=" + imageryTypeID;
  }

  if (!sendMessage(
        this->Connection, "GET", "/integrate/2/imagery?" + urldata, headers, frmdata, result))
  {
    vtkGenericWarningMacro("Failed to request matching imagery for: " << urldata);
    return false;
  }
  resultJSON = getJSON(result);

  this->ImageryMap[key] = resultJSON;

  if (!resultJSON.isObject() || !resultJSON.isMember("imageries") ||
    resultJSON["imageries"].size() == 0)
  {
    return false;
  }

  return true;
}

bool vtkImagoLoader::GetImage(std::string const& workspace, std::string const& dataset,
  std::string const& collection, std::string const& imageryType, std::string const& imageType,
  std::string const& depth, std::future<vtkImageData*>& future)
{
  // make sure we are connected, have a token, and have gotten the context
  if (!this->IsLoggedIn())
  {
    return false;
  }

  // if we have a workspace then get the id
  std::string workspaceID;
  if (workspace.size())
  {
    for (auto& w : this->ContextJSON["workspaces"])
    {
      if (workspace == w["name"].asString())
      {
        workspaceID = w["id"].asString();
      }
    }
  }

  // async call to get the image as this can take a while
  // and we don't want to block the VR session

  future = std::async(std::launch::async,
    [this, collection, workspace, depth, dataset, imageType, imageryType]() -> vtkImageData* {
      // if we have a dataset find the id
      std::string datasetID;
      if (dataset.size())
      {
        for (auto& w : this->ContextJSON["workspaces"])
        {
          // is this the correct workspace
          if (workspace.size() == 0 || this->Workspace == w["name"].asString())
          {
            auto& ds = w["datasets"];
            for (auto& d : ds)
            {
              if (dataset == d["name"].asString())
              {
                datasetID = d["id"].asString();
              }
            }
          }
        }
      }

      // force these async calls to be serial, eg they are asyn to the main thread but serial
      // when multiple invocations
      std::lock_guard<std::mutex> lock(this->Mutex);

      // search for a matching collection
      Json::Value collectionJSON;
      if (!this->GetCollection(collection, datasetID, collectionJSON))
      {
        return nullptr;
      }
      std::string collectionID = collectionJSON["collections"][0]["id"].asString();

      // if we don't have a datasetid yet, then get it now
      if (!datasetID.size())
      {
        datasetID = collectionJSON["collections"][0]["datasetId"].asString();
      }

      // at this point we have the collectionID and datasetID and maybe a workspaceID

      // now find the imagerytype id from the context and datasetid
      std::vector<std::string> imageryTypeIDs;
      std::vector<std::string> imageTypeIDs;
      for (auto& w : this->ContextJSON["workspaces"])
      {
        auto& ds = w["datasets"];
        for (auto& d : ds)
        {
          // is this the correct dataset?
          if (datasetID == d["id"].asString())
          {
            auto& its = d["imageryTypes"];
            for (auto& it : its)
            {
              if (imageryType.size() == 0 || it["name"].asString() == imageryType)
              {
                auto& imageTypes = it["imageTypes"];
                for (auto& iType : imageTypes)
                {
                  if (imageType.size() == 0 || iType["name"].asString() == imageType)
                  {
                    // always set these two together so they match
                    imageryTypeIDs.push_back(it["id"].asString());
                    imageTypeIDs.push_back(iType["id"].asString());
                  }
                }
              }
            }
          }
        }
      }

      // see if we have a match
      std::string imageTypeID;
      std::string imageryTypeID;
      Json::Value imageryJSON;
      for (size_t i = 0; i < imageryTypeIDs.size(); ++i)
      {
        Json::Value tmpJSON;
        if (this->GetImagery(collectionID, imageryTypeIDs[i], tmpJSON))
        {
          imageryTypeID = imageryTypeIDs[i];
          imageTypeID = imageTypeIDs[i];
          imageryJSON = tmpJSON;
        }
      }

      // if nothing found error
      if (imageryTypeID.size() == 0)
      {
        // this can happen when someone has requested a specific image type
        // but the current drillhole doesn't have that image type
        // and that is OK
        return nullptr;
      }

      // find the image we want based on the depth
      double targetDepth = std::stod(depth.c_str());
      double startDepth = 0.0;
      double endDepth = 0.0;
      // record last depth requested
      this->LastStartDepth = startDepth;
      this->LastEndDepth = endDepth;
      size_t fileSize = 0;
      std::string imageryID;
      std::string mimeType;
      for (auto& i : imageryJSON["imageries"])
      {
        startDepth = i["startDepth"].asDouble();
        endDepth = i["endDepth"].asDouble();
        if (targetDepth >= startDepth && targetDepth < endDepth)
        {
          std::string sFileSize = i["images"][0]["fileSize"].asString();
          fileSize = std::atol(sFileSize.c_str());
          imageryID = i["id"].asString();
          mimeType = i["images"][0]["mimeType"].asString();
          // finally get the image
          if (!imageryID.size() || !imageTypeID.size())
          {
            vtkGenericWarningMacro("Failed to find matching imagery");
            return nullptr;
          }
          break;
        }
      }

      // is the image cached?
      std::string imageString = imageryID + imageTypeID;
      for (auto& p : this->ImageCache)
      {
        if (p.first == imageString)
        {
          p.second.Get()->Register(nullptr);

          // record last depth requested
          this->LastStartDepth = startDepth;
          this->LastEndDepth = endDepth;
          return p.second;
        }
      }

      if (mimeType != "image/jpeg")
      {
        vtkGenericWarningMacro("Format not supported for type " << mimeType);
        return nullptr;
      }

      vtkImageData* imageData = nullptr;

      std::vector<char> result;

      // finally get the actual image
      if (!this->AcquireToken())
      {
        return nullptr;
      }

      std::vector<std::string> headers;
      headers.push_back("Content-Type: image/jpeg\r\n");
      headers.push_back("imago-api-token: " + this->APIToken + "\r\n");

      std::string frmdata;
      // now search for the imageryID and mimeType for the image
      std::string urldata = "imageryid=" + imageryID + "&imagetypeid=" + imageTypeID;

      int tries = 0;
      bool success = false;
      do
      {
        if (!sendMessage(
              this->Connection, "GET", "/integrate/2/image?" + urldata, headers, frmdata, result))
        {
          vtkGenericWarningMacro("Failed to find matching image for: " << urldata);
          return nullptr;
        }
        if (fileSize == result.size())
        {
          // stick it into a reader
          vtkNew<vtkJPEGReader> rdr;
          rdr->SetMemoryBuffer(result.data());
          rdr->SetMemoryBufferLength(result.size());
          rdr->Update();
          if (rdr->GetErrorCode() == 0)
          {
            imageData = rdr->GetOutput();
            imageData->Register(nullptr);
            success = true;
          }
        }
        tries++;
      } while (tries < 5 && !success);

      // did we succeed
      if (!success)
      {
        vtkGenericWarningMacro("failed to retrieve file");
        return nullptr;
      }

      if (this->ImageCache.size() > this->CacheLimit)
      {
        this->ImageCache.pop_front();
      }

      this->ImageCache.push_back(
        std::pair<std::string, vtkSmartPointer<vtkImageData> >(imageString, imageData));

      // record last depth requested
      this->LastStartDepth = startDepth;
      this->LastEndDepth = endDepth;
      return imageData;
    });

  return true;
}
