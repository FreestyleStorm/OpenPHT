#include "PlexFile.h"
#include "Client/PlexServerManager.h"
#include "utils/log.h"
#include "settings/GUISettings.h"
#include "boost/lexical_cast.hpp"
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <string>

#include "PlexApplication.h"

using namespace XFILE;
using namespace std;

typedef pair<string, string> stringPair;

vector<stringPair> CPlexFile::GetHeaderList()
{
  std::vector<std::pair<std::string, std::string> > hdrs;
  
  hdrs.push_back(stringPair("X-Plex-Version", PLEX_VERSION));

  hdrs.push_back(stringPair("X-Plex-Client-Identifier", g_guiSettings.GetString("system.uuid")));
  hdrs.push_back(stringPair("X-Plex-Provides", "player"));
  hdrs.push_back(stringPair("X-Plex-Product", "Plex Home Theater"));
  hdrs.push_back(stringPair("X-Plex-Device-Name", g_guiSettings.GetString("services.devicename")));
  
  hdrs.push_back(stringPair("X-Plex-Platform", "Plex Home Theater"));
  hdrs.push_back(stringPair("X-Plex-Model", PlexUtils::GetMachinePlatform()));
  hdrs.push_back(stringPair("X-Plex-Capabilities", "ehls,rtmp,multipart"));
#ifdef TARGET_RPI
  hdrs.push_back(stringPair("X-Plex-Device", "RaspberryPi"));
#elif defined(TARGET_DARWIN_IOS)
  hdrs.push_back(stringPair("X-Plex-Device", "AppleTV"));
#else
  hdrs.push_back(stringPair("X-Plex-Device", "PC"));
#endif
  
  if (g_plexApplication.myPlexManager->IsSignedIn())
    hdrs.push_back(stringPair("X-Plex-Username", g_plexApplication.myPlexManager->GetCurrentUserInfo().username));

  return hdrs;
}

CPlexFile::CPlexFile(void) : CCurlFile()
{
  BOOST_FOREACH(stringPair sp, GetHeaderList())
    SetRequestHeader(sp.first, sp.second);
}

bool
CPlexFile::BuildHTTPURL(CURL& url)
{
  CURL newUrl;
  CPlexServerPtr server;
  CStdString key;

  /* allow passthrough */
  if (url.GetProtocol() != "plexserver")
    return true;

  if (!g_plexApplication.serverManager)
    return false;

  if (PlexUtils::IsValidIP(url.GetHostName()))
  {
    server = g_plexApplication.serverManager->FindByHostAndPort(url.GetHostName(), url.GetPort());
    key = url.GetHostName() + ":" + boost::lexical_cast<CStdString>(url.GetPort());
  }
  else
  {
    key = url.GetHostName();
    server = g_plexApplication.serverManager->FindByUUID(key);
  }

  if (!server)
  {
    /* Ouch, this should not happen! */
    CLog::Log(LOGWARNING, "CPlexFile::BuildHTTPURL tried to lookup server %s but it was not found!", key.c_str());
    return false;
  }

  newUrl = server->BuildURL(url.GetFileName(), url.GetOptions());

  if (!url.GetUserName().empty())
    newUrl.SetUserName(url.GetUserName());
  if (!url.GetPassWord().empty())
    newUrl.SetPassword(url.GetPassWord());

  CLog::Log(LOGDEBUG, "CPlexFile::BuildHTTPURL translated '%s' to '%s'", url.Get().c_str(), newUrl.Get().c_str());
  url = newUrl;

  return true;
}

bool
CPlexFile::Open(const CURL &url)
{
  CURL newUrl(url);
  if (BuildHTTPURL(newUrl))
    return CCurlFile::Open(newUrl);
  return false;
}

int
CPlexFile::Stat(const CURL &url, struct __stat64 *buffer)
{
  CURL newUrl(url);
  if (BuildHTTPURL(newUrl))
    return CCurlFile::Stat(newUrl, buffer);
  return false;
}

bool
CPlexFile::Exists(const CURL &url)
{
  CURL newUrl(url);
  if (BuildHTTPURL(newUrl))
    return CCurlFile::Exists(newUrl);
  return false;
}
