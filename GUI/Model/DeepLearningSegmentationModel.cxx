#include "DeepLearningSegmentationModel.h"
#include "json/json.h"
#include "GenericImageData.h"
#include "GlobalUIModel.h"
#include "PropertyModel.h"
#include "RESTClient.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include "UIReporterDelegates.h"
#include "SegmentationUpdateIterator.h"
#include "AllPurposeProgressAccumulator.h"
#include "base64.h"
#include "QtCursorOverride.h"
#include <chrono>
#include <regex>
#include <functional>
#include "itksys/MD5.h"

// Minimum server version
const char *DeepLearningSegmentationModel::MINIMUM_SERVER_VERSION = "0.1.0";

typedef std::chrono::high_resolution_clock Clock;


#if defined(ITKZLIB) && !defined(ITK_USE_SYSTEM_ZLIB)
#include "itk_zlib.h"
#else
#include "zlib.h"
#endif

#include "libssh/libssh.h"

bool
startSshTunnel(const std::string &remoteHost,
               int                remotePort,
               int                localPort,
               const std::string &username,
               const std::string &password,
               const std::string &private_key)
{
  ssh_session session = ssh_new();
  if (!session)
  {
    return false;
  }

  ssh_options_set(session, SSH_OPTIONS_HOST, remoteHost.c_str());
  ssh_options_set(session, SSH_OPTIONS_USER, username.c_str());

  if (ssh_connect(session) != SSH_OK)
  {
    std::cerr << "SSH Connection failed: " << ssh_get_error(session) << std::endl;
    ssh_free(session);
    return false;
  }

  // Authenticate using the private key
  bool auth = false;
  if (ssh_userauth_publickey_auto(session, NULL, NULL) == SSH_AUTH_SUCCESS)
  {
    std::cout << "Successfully authenticated using auto-detected key" << std::endl;
    auth = true;
  }
  else if (ssh_userauth_password(session, nullptr, password.c_str()) == SSH_AUTH_SUCCESS)
  {
    std::cout << "Successfully authenticated using password" << std::endl;
    auth = true;
  }
  else
  {
    std::cerr << "Authentication failed: " << ssh_get_error(session) << std::endl;
    ssh_disconnect(session);
    ssh_free(session);
    return false;
  }

  // Open a direct TCP/IP tunnel
  ssh_channel channel = ssh_channel_new(session);
  if (!channel)
  {
    ssh_disconnect(session);
    ssh_free(session);
    return false;
  }

  if (ssh_channel_open_forward(channel, "localhost", remotePort, "localhost", localPort) != SSH_OK)
  {
    std::cerr << "Failed to open tunnel: " << ssh_get_error(session) << std::endl;
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
    return false;
  }

  return true; // Tunnel is now open
}


bool gzipInflate( const std::string& compressedBytes, std::string& uncompressedBytes )
{
  if ( compressedBytes.size() == 0 )
  {
    uncompressedBytes = compressedBytes ;
    return true ;
  }

  uncompressedBytes.clear();
  constexpr unsigned buffer_size = 1024 * 1024;
  std::vector<unsigned char> buffer(buffer_size);

  z_stream strm;
  strm.next_in = (Bytef *) compressedBytes.c_str();
  strm.avail_in = compressedBytes.size() ;
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false ;

  if (inflateInit2(&strm, (16+MAX_WBITS)) != Z_OK)
  {
    return false;
  }

  while (!done)
  {
    strm.next_out = buffer.data();
    strm.avail_out = buffer_size;

           // strm.next_out = (Bytef *) (uncomp + strm.total_out);
           // strm.avail_out = uncompLength - strm.total_out;

           // Inflate another chunk.
    size_t n0 = strm.total_out;
    int err = inflate (&strm, Z_SYNC_FLUSH);
    size_t n = strm.total_out - n0;

           // Paste the read bytes
    uncompressedBytes.append((char *) buffer.data(), n);

    if (err == Z_STREAM_END) done = true;
    else if (err != Z_OK)  {
      break;
    }
  }

  if (inflateEnd (&strm) != Z_OK) {
    return false;
  }

  return true ;
}

bool gzipDeflate(const char *uncompressedBytes, size_t n_bytes, std::string& compressedBytes)
{
  compressedBytes.clear();
  if (n_bytes == 0)
  {
    return true;
  }

  constexpr unsigned buffer_size = 1024 * 1024;
  std::vector<unsigned char> buffer(buffer_size);

  z_stream strm;
  strm.next_in = (Bytef *) uncompressedBytes;
  strm.avail_in = n_bytes;
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false ;

  if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
  {
    return false;
  }

  while (!done)
  {
    strm.next_out = buffer.data();
    strm.avail_out = buffer_size;
    size_t n0 = strm.total_out;
    int err = deflate(&strm, Z_FINISH);
    size_t n = strm.total_out - n0;

    // Paste the read bytes
    compressedBytes.append((char *) buffer.data(), n);

    if (err == Z_STREAM_END) done = true;
    else if (err != Z_OK)  {
      break;
    }
  }

  if (deflateEnd(&strm) != Z_OK) {
    return false;
  }

  return true ;
}

template <class T>
std::string get_numpy_type()
{
  auto &type = typeid(T);
  if (type == typeid(int8_t))    return "int8";
  if (type == typeid(uint8_t))   return "uint8";
  if (type == typeid(int16_t))   return "int16";
  if (type == typeid(uint16_t))  return "uint16";
  if (type == typeid(int32_t))   return "int32";
  if (type == typeid(uint32_t))  return "uint32";
  if (type == typeid(int64_t))   return "int64";
  if (type == typeid(uint64_t))  return "uint64";
  if (type == typeid(float))     return "float32";
  if (type == typeid(double))    return "float64";
  return "unknown";
}

template <class TImage>
void EncodeImage(RESTMultipartData &mpd, TImage *image, std::string &buffer_storage, double reserve_ratio = 1.0)
{
  // Encode the raw data
  image->Update();
  size_t n_bytes_raw = image->GetPixelContainer()->Size() * sizeof(typename TImage::InternalPixelType);
  buffer_storage.reserve((size_t) (n_bytes_raw * reserve_ratio));
  gzipDeflate((char*)image->GetBufferPointer(), n_bytes_raw, buffer_storage);
  mpd.addBytes("file", "application/gzip", "image.gz", buffer_storage.c_str(), buffer_storage.size());

  // Generate json with the image metadata
  Json::Value root(Json::objectValue);
  root["dimensions"] = Json::Value(Json::arrayValue);
  root["spacing"] = Json::Value(Json::arrayValue);
  root["origin"] = Json::Value(Json::arrayValue);
  root["direction"] = Json::Value(Json::arrayValue);
  root["components_per_pixel"] = Json::Value(image->GetNumberOfComponentsPerPixel());
  root["component_type"] = get_numpy_type<typename TImage::InternalPixelType>();
  for(unsigned int i = 0; i < TImage::ImageDimension; i++)
  {
    root["dimensions"].append((int) image->GetBufferedRegion().GetSize()[i]);
    root["origin"].append((double) image->GetOrigin()[i]);
    root["spacing"].append((double) image->GetSpacing()[i]);
    for(unsigned int j = 0; j < TImage::ImageDimension; j++)
      root["direction"].append((double)image->GetDirection()(i,j));
  }
  std::ostringstream oss;
  oss << root;
  mpd.addString("metadata", "application/json", oss.str());
}


DeepLearningSegmentationModel::DeepLearningSegmentationModel()
{
  m_ServerModel =
    wrapGetterSetterPairAsProperty(this, &Self::GetServerValueAndRange, &Self::SetServerValue);

  m_IsActiveModel = NewSimpleConcreteProperty(false);
  this->Rebroadcast(m_IsActiveModel, ValueChangedEvent(), ServerChangeEvent());

  m_IsServerConfiguredModel = wrapGetterSetterPairAsProperty(this, &Self::GetServerIsConfiguredValue);
  m_IsServerConfiguredModel->RebroadcastFromSourceProperty(m_ServerModel);

  m_ServerDisplayURLModel = wrapGetterSetterPairAsProperty(this, &Self::GetServerDisplayURLValue);
  m_ServerDisplayURLModel->RebroadcastFromSourceProperty(m_ServerModel);

  m_ProxyURLModel = NewSimpleConcreteProperty(std::string());

  // Substatuses
  m_NetworkStatusModel =
    NewSimpleConcreteProperty(dls_model::NetworkStatus(dls_model::NETWORK_NOT_CONNECTED));
  m_TunnelStatusModel =
    NewSimpleConcreteProperty(dls_model::TunnelStatus(dls_model::TUNNEL_ESTABLISHING));
  m_LocalServerStatusModel =
    NewSimpleConcreteProperty(dls_model::LocalServerStatus(dls_model::LOCAL_SERVER_STARTING));

  // Server status model
  m_ServerStatusModel =
    wrapGetterSetterPairAsProperty(this, &Self::GetServerStatusValue);
  m_ServerStatusModel->RebroadcastFromSourceProperty(m_NetworkStatusModel);
  m_ServerStatusModel->RebroadcastFromSourceProperty(m_TunnelStatusModel);
  m_ServerStatusModel->RebroadcastFromSourceProperty(m_LocalServerStatusModel);
  m_ServerStatusModel->Rebroadcast(m_ServerModel, ValueChangedEvent(), ValueChangedEvent());
  m_ServerStatusModel->Rebroadcast(m_ServerModel, DomainChangedEvent(), ValueChangedEvent());

  m_ServerProgressModel = NewRangedConcreteProperty(0., 0., 0., 0.);

  // Set up the progress command
  m_ProgressCommand = itk::MemberCommand<Self>::New();
  m_ProgressCommand->SetCallbackFunction(this, &Self::ProgressCallback);

  this->Rebroadcast(m_ServerModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerModel, DomainChangedEvent(), ServerChangeEvent());

  // Initialize the cookie jar
  m_RESTSharedData = new RESTSharedDataType();
}

DeepLearningSegmentationModel::~DeepLearningSegmentationModel()
{
  delete m_RESTSharedData;
}

dls_model::ConnectionStatus
DeepLearningSegmentationModel::MergeStatuses()
{
  auto *sp = GetServerProperties();
  auto status_net = this->GetNetworkStatus();
  auto status_tun = this->GetTunnelStatus();
  auto status_loc = this->GetLocalServerStatus();

  if(sp && sp->GetRemoteConnection())
  {
    if(sp->GetUseSSHTunnel())
    {
      switch(status_net.status)
      {
        case dls_model::NETWORK_NOT_CONNECTED:
        case dls_model::NETWORK_CHECKING:
          switch(status_tun.status)
          {
            case dls_model::TUNNEL_ESTABLISHING:
              return dls_model::ConnectionStatus(dls_model::CONN_TUNNEL_ESTABLISHING);
            case dls_model::TUNNEL_FAILED:
              return dls_model::ConnectionStatus(dls_model::CONN_TUNNEL_FAILED, status_tun.message);
            case dls_model::TUNNEL_CREATED:
              if(status_net.status == dls_model::NETWORK_CHECKING)
                return dls_model::ConnectionStatus(dls_model::CONN_CHECKING);
              else
                return dls_model::ConnectionStatus(dls_model::CONN_NOT_CONNECTED, status_net.message);
          }
        case dls_model::NETWORK_CONNECTED_COMPATIBLE:
          return dls_model::ConnectionStatus(dls_model::CONN_CONNECTED, status_net.message);
        case dls_model::NETWORK_CONNECTED_INCOMPATIBLE:
          return dls_model::ConnectionStatus(dls_model::CONN_INCOMPATIBLE, status_net.message);
      }
    }
    else
    {
      switch(status_net.status)
      {
        case dls_model::NETWORK_NOT_CONNECTED:
          return dls_model::ConnectionStatus(dls_model::CONN_NOT_CONNECTED, status_net.message);
        case dls_model::NETWORK_CHECKING:
          return  dls_model::ConnectionStatus(dls_model::CONN_CHECKING);
        case dls_model::NETWORK_CONNECTED_COMPATIBLE:
          return  dls_model::ConnectionStatus(dls_model::CONN_CONNECTED, status_net.message);
        case dls_model::NETWORK_CONNECTED_INCOMPATIBLE:
          return  dls_model::ConnectionStatus(dls_model::CONN_INCOMPATIBLE, status_net.message);
      }
    }
  }
  else if(sp)
  {
    switch(status_loc.status)
    {
      case dls_model::LOCAL_SERVER_FAILED:
        return dls_model::ConnectionStatus(dls_model::CONN_LOCAL_SERVER_FAILED, status_net.message);
      case dls_model::LOCAL_SERVER_STARTING:
        switch(status_net.status)
        {
          case dls_model::NETWORK_NOT_CONNECTED:
          case dls_model::NETWORK_CHECKING:
            return dls_model::ConnectionStatus(dls_model::CONN_LOCAL_SERVER_STARTING);
          case dls_model::NETWORK_CONNECTED_COMPATIBLE:
            return dls_model::ConnectionStatus(dls_model::CONN_CONNECTED, status_net.message);
          case dls_model::NETWORK_CONNECTED_INCOMPATIBLE:
            return dls_model::ConnectionStatus(dls_model::CONN_INCOMPATIBLE, status_net.message);
        }
      case dls_model::LOCAL_SERVER_ESTABLISHED:
        switch(status_net.status)
        {
          case dls_model::NETWORK_NOT_CONNECTED:
            return dls_model::ConnectionStatus(dls_model::CONN_NOT_CONNECTED, status_net.message);
          case dls_model::NETWORK_CHECKING:
            return dls_model::ConnectionStatus(dls_model::CONN_LOCAL_SERVER_STARTING);
          case dls_model::NETWORK_CONNECTED_COMPATIBLE:
            return dls_model::ConnectionStatus(dls_model::CONN_CONNECTED, status_net.message);
          case dls_model::NETWORK_CONNECTED_INCOMPATIBLE:
            return dls_model::ConnectionStatus(dls_model::CONN_INCOMPATIBLE, status_net.message);
        }
    }
  }
  else
  {
    return dls_model::ConnectionStatus(dls_model::CONN_NO_SERVER);
  }
}

bool
DeepLearningSegmentationModel::GetServerStatusValue(dls_model::ConnectionStatus &value)
{
  value = MergeStatuses();
  return true;
}

bool
DeepLearningSegmentationModel::GetServerIsConfiguredValue(bool &value)
{
  value = (m_ServerIndex >= 0 && m_ServerIndex < m_ServerProperties.size());
  return true;
}

bool
DeepLearningSegmentationModel::GetServerDisplayURLValue(std::string &value)
{
  if(m_ServerIndex >= 0 && m_ServerIndex < m_ServerProperties.size())
    value = m_ServerProperties[m_ServerIndex]->GetFullURL();
  else
    value = std::string();
  return true;
}

void
DeepLearningSegmentationModel::ProgressCallback(Object *source, const itk::EventObject &event)
{
  itk::ProcessObject *po = static_cast<itk::ProcessObject *>(source);
  if(po)
  {
    this->m_ServerProgressModel->SetValue(po->GetProgress());
    this->m_ServerProgressModel->SetDomain(NumericValueRange<double>(0,1,0.001));
    std::cout << "Progress: " << po->GetProgress() << std::endl;
  }
}

bool
DeepLearningSegmentationModel::GetServerValueAndRange(int &value, ServerDomain *domain)
{
  value = m_ServerIndex;
  if(domain)
    domain->SetWrappedVector(&m_ServerProperties);
  return true;
}

void
DeepLearningSegmentationModel::ResetConnection()
{
  // Reset the proxy URL
  SetProxyURL(std::string());

  // Reset the statuses to default state
  SetNetworkStatus(dls_model::NetworkStatus());
  SetTunnelStatus(dls_model::TunnelStatus());
  SetLocalServerStatus(dls_model::LocalServerStatus());
}

void
DeepLearningSegmentationModel::SetServerValue(int value)
{
  // Check if the value is in range
  itkAssertOrThrowMacro(value == -1 || (value >= 0 && value < m_ServerProperties.size()),
                        "Incorrect value in DeepLearningSegmentationModel::SetServerValue");

  // Set the current server index
  m_ServerIndex = value;

  // Reset the session variables - we are starting over
  m_ActiveSession = std::string();
  m_ActiveSessionModel = std::string();

  // Reset the connection
  ResetConnection();

  // Fire some events
  m_ServerModel->InvokeEvent(ValueChangedEvent());
}

void
DeepLearningSegmentationModel::SetLocalServerDelegate(AbstractLocalDeepLearningServerDelegate *delegate)
{
  m_LocalServerDelegate = delegate;
}

void
DeepLearningSegmentationModel::SetParentModel(GlobalUIModel *parent)
{
  m_ParentModel = parent;
}

void
DeepLearningSegmentationModel::LoadPreferences(Registry &folder)
{
  // Clear the server list
  m_ServerProperties.clear();

  // Read the available server names
  auto &server_folder = folder.Folder("Servers");
  for (int k = 0; k < server_folder["ArraySize"][0]; k++)
  {
    Registry &f = server_folder.Folder(server_folder.Key("Element[%d]", k));
    SmartPtr<DeepLearningServerPropertiesModel> spm = DeepLearningServerPropertiesModel::New();
    spm->ReadFromRegistry(f);
    m_ServerProperties.push_back(spm);
  }

  // Read the active server index
  int server_index = folder["ActiveServerIndex"][0];
  if(server_index < 0 || server_index >= m_ServerProperties.size())
    server_index = m_ServerProperties.size() ? 0 : -1;

  // Set the current server - and all that goes with that
  m_ServerModel->SetValue(server_index);

  // Not only the value, but the domain has changed
  m_ServerModel->InvokeEvent(DomainChangedEvent());
}

void
DeepLearningSegmentationModel::SavePreferences(Registry &folder)
{
  // Clear the list of servers
  auto &server_folder = folder.Folder("Servers");
  server_folder.Clear();

  // Write each server
  server_folder["ArraySize"] << m_ServerProperties.size();
  for(int k = 0; k < m_ServerProperties.size(); k++)
  {
    Registry &f = server_folder.Folder(server_folder.Key("Element[%d]", k));
    m_ServerProperties[k]->WriteToRegistry(f);
  }

  // Write the selected server
  folder["ActiveServerIndex"] << m_ServerIndex;
}

DeepLearningServerPropertiesModel *
DeepLearningSegmentationModel::GetServerProperties()
{
  return m_ServerIndex >= 0 && m_ServerIndex < m_ServerProperties.size()
           ? m_ServerProperties[m_ServerIndex]
           : nullptr;
}

void
DeepLearningSegmentationModel::UpdateServerProperties(DeepLearningServerPropertiesModel *model,
                                                      bool add_as_new)
{
  itkAssertOrThrowMacro(
    add_as_new || (m_ServerIndex >= 0 && m_ServerIndex < m_ServerProperties.size()),
    "Incorrect state in DeepLearningSegmentationModel::UpdateServerProperties");

  if (add_as_new)
  {
    m_ServerProperties.push_back(model);
    SetServerValue(m_ServerProperties.size() - 1);
  }
  else
  {
    m_ServerProperties[m_ServerIndex] = model;
    SetServerValue(m_ServerIndex);
  }

  m_ServerModel->InvokeEvent(DomainChangedEvent());
}

void
DeepLearningSegmentationModel::DeleteCurrentServer()
{
  itkAssertOrThrowMacro(
    m_ServerIndex >= 0 && m_ServerIndex < m_ServerProperties.size(),
    "Incorrect state in DeepLearningSegmentationModel::DeleteCurrentServer");

  // Remove the properties
  m_ServerProperties.erase(m_ServerProperties.begin() + m_ServerIndex);
  if(m_ServerIndex >= m_ServerProperties.size())
    m_ServerIndex = m_ServerProperties.size() - 1;

  // Update the server
  SetServerValue(m_ServerIndex);

  m_ServerModel->InvokeEvent(DomainChangedEvent());
}

void
DeepLearningSegmentationModel::StartLocalServerIfNeeded()
{
  itkAssertOrThrowMacro(m_LocalServerDelegate, "DeepLearningSegmentationModel::StartLocalServerIfNeeded called without a delegate assigned");
  int port = m_LocalServerDelegate->StartServerIfNeeded(this->GetServerProperties());
  if(port > 0)
  {
    m_LocalPortNumber = port;
    this->SetLocalServerStatus(dls_model::LocalServerStatus(dls_model::LOCAL_SERVER_STARTING));
  }
  else if(port < 0)
  {
    m_LocalPortNumber = -1;
    this->SetLocalServerStatus(dls_model::LocalServerStatus(dls_model::LOCAL_SERVER_FAILED));
  }
}

static std::vector<int>
parse_version(const std::string &v)
{
  std::vector<int>  parts;
  std::stringstream ss(v);
  std::string       item;
  while (std::getline(ss, item, '.'))
  {
    parts.push_back(std::stoi(item));
  }
  return parts;
}

static int
compare_versions(const std::string &a, const std::string &b)
{
  std::vector<int> A = parse_version(a);
  std::vector<int> B = parse_version(b);

  size_t n = std::max(A.size(), B.size());
  A.resize(n, 0);
  B.resize(n, 0);

  for (size_t i = 0; i < n; ++i)
  {
    if (A[i] < B[i])
      return -1;
    if (A[i] > B[i])
      return 1;
  }
  return 0;
}

// Returns true if version >= min_required.
bool
version_meets_min(const std::string &version, const std::string &min_required)
{
  return compare_versions(version, min_required) >= 0;
}


DeepLearningSegmentationModel::StatusCheck
DeepLearningSegmentationModel::AsyncCheckStatus()
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  dls_model::NetworkStatus response;
  auto *sp = this->GetServerProperties();

  // Check if URL has been set
  if (!sp)
  {
    return std::make_pair(std::string(), response);
  }

  // Check if the proxy has been set up for an SSH connection
  if (sp->GetUseSSHTunnel() && GetProxyURL().size() == 0)
  {
    response.status = dls_model::NETWORK_NOT_CONNECTED;
    response.message = "SSH tunnel has not been configured";
  }
  else
  {
    try
    {
      RESTClientType cli(m_RESTSharedData);
      std::string url = GetActualServerURL();
      cli.SetServerURL(url.c_str());
      bool status = cli.Get("status");
      if (!status)
      {
        response.status = dls_model::NETWORK_NOT_CONNECTED;
        response.message = cli.GetErrorString();

        // Special handling for ngrok server responses
        std::regex re_ngrok(R"(ngrok.*app)", std::regex::icase);
        if(std::regex_search(url, re_ngrok))
        {
          std::regex noscriptRegex(R"(<noscript>(.*?)</noscript>)", std::regex::icase);
          std::smatch match;
          if (std::regex_search(response.message, match, noscriptRegex) && match.size() > 1)
            response.message = match[1].str();
        }
      }
      else
      {
        Json::Reader json_reader;
        Json::Value  root;
        if (json_reader.parse(cli.GetOutput(), root, false))
        {
          // Check the version against minimum version
          auto version = root["version"].asString();
          if(version_meets_min(version, MINIMUM_SERVER_VERSION))
          {
            response.status = dls_model::NETWORK_CONNECTED_COMPATIBLE;
            response.message = version;
          }
          else
          {
            response.status = dls_model::NETWORK_CONNECTED_INCOMPATIBLE;
            response.message = version;
          }
        }
        else
          throw IRISException("Server did not provide version number");
      }
    }
    catch (IRISException &exc)
    {
      // TODO: this is a bad workaround the fact that we don't know when the local
      response.status = dls_model::NETWORK_NOT_CONNECTED;
      response.message = exc.what();
    }
  }

  return std::make_pair(sp->GetHash(), response);
}

void
DeepLearningSegmentationModel::ApplyStatusCheckResponse(const StatusCheck &result)
{
  std::string hash = m_ServerIndex < 0 ? std::string() : m_ServerProperties[m_ServerIndex]->GetHash();
  if(hash == result.first)
    this->SetNetworkStatus(result.second);

}

std::map<std::string, dls_model::RemoteModelMetadata>
DeepLearningSegmentationModel::GetRemotePipelines()
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once
  if(this->GetServerStatus().status != dls_model::CONN_CONNECTED)
  {
    m_RemoteModelMetadata.clear();
    return m_RemoteModelMetadata;
  }

  if(m_RemoteModelMetadata.empty())
  {
    try
    {
      RESTClientType cli(m_RESTSharedData);
      cli.SetServerURL(GetActualServerURL().c_str());
      if(!cli.Get("v2/models"))
        throw IRISException("Error requesting list of models from DLS server: %s", cli.GetErrorString());

      Json::Reader json_reader;
      Json::Value root;
      if(json_reader.parse(cli.GetOutput(), root, false))
      {
        const Json::Value models_list = root["models"];
        for(auto m : models_list)
        {
          dls_model::RemoteModelMetadata md;

          // Read model id
          auto model_id = m["id"].asString();

          // Read model dimensionality
          md.dimensions = m["dimensions"].asInt();

          // Read number of channels supported
          for(auto chan : m["channels"])
          {
            int nchan = chan.asInt();
            md.channels.insert(nchan);
          }

          for(auto inter : m["interactions"])
          {
            auto itype = inter.asString();
            if(itype == "point")
              md.supports_point = true;
            if(itype == "box")
              md.supports_box = true;
            if(itype == "lasso")
              md.supports_lasso = true;
            if(itype == "scribble")
              md.supports_scribble = true;
          }
          m_RemoteModelMetadata[model_id] = md;
        }
      }
      else
        throw IRISException("Error requesting list of models from DLS server; unexpected return value: '%s'", cli.GetOutput());
    }
    catch(std::exception &exc)
    {
      m_RemoteModelMetadata.clear();
      throw IRISException("Error requesting list of models from DLS server: %s", exc.what());
    }
  }
  return m_RemoteModelMetadata;
}

void
DeepLearningSegmentationModel::SetSourceImage(const std::string &model_id, ImageWrapperBase *layer, unsigned int axis)
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  // Set waiting cursor for DLS operation
  QtCursorOverride c(Qt::WaitCursor);
  
  RESTClientType cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());

  // The model has to exist
  auto &model_metadata = GetRemoteModelMetadata(model_id);

  // If there is an active session and the user wants to use a new model, release active session
  if(m_ActiveSession.size() && m_ActiveSessionModel != model_id)
  {
    ProgressTaskGuard task_tracker(m_ProgressDelegate, "Ending prior session");
    cli.Get("v2/end_session/%s", m_ActiveSession.c_str());
    m_ActiveSession.clear();
    m_ActiveSessionModel.clear();
    m_RemoteImageViewHash = 0;
  }

  // Check if we have an open session with the server, if not establish it
  if(m_ActiveSession.size() == 0)
  {
    ProgressTaskGuard task_tracker(m_ProgressDelegate, "Creating DLS session");
    cli.SetProgressCallback(&task_tracker, &ProgressTaskGuard::ProgressCallback);
    Clock::time_point t0, t1;
    t0 = Clock::now();
    if(!cli.Get("v2/start_session/%s", model_id.c_str()))
      throw IRISException("Error creating session on DLS server: %s", cli.GetErrorString());
    cli.RemoveProgressCallback();
    t1 = Clock::now();

    Json::Reader json_reader;
    Json::Value root;
    if(json_reader.parse(cli.GetOutput(), root, false))
    {
      const Json::Value result = root["session_id"];
      m_ActiveSession = result.asString();
      m_ActiveSessionModel = model_id;
    }
    else
      throw IRISException("Error creating session on DSL server: unexpected return value '%s'", cli.GetOutput());

    std::cout << "Initialize session time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms "
              << std::endl;
  }

  // Send the current image to the server
  auto *driver = m_ParentModel->GetDriver();
  unsigned int tp = driver->GetCursorTimePoint();

  // The layer we apply segmentation to should be chosen depending on what number of
  // channels the model supports, and the current display mode of the layer.
  auto *policy = dynamic_cast<AbstractMultiChannelDisplayMappingPolicy *>(layer->GetDisplayMapping());
  ImageWrapperBase *actual_layer = layer;
  if(!layer->IsScalar())
  {
    if(policy->GetDisplayMode().UseRGB && model_metadata.supports_channels(3))
      actual_layer = layer;
    else
      actual_layer = layer->GetDefaultScalarRepresentation();
  }

  // Generate a hash that describes the parameters of the image or image region that we
  // would be sending to the server, without actually generating the image (not to waste
  // time).
  size_t rview_hash = std::hash<unsigned long>{}(actual_layer->GetUniqueId());
  rview_hash ^= std::hash<unsigned int>{}(actual_layer->GetTimePointIndex());
  if(model_metadata.dimensions == 2)
  {
    auto zpos = layer->MapImageCIndexToSliceCIndex(axis, to_double(driver->GetCursorPosition()))[2];
    rview_hash ^= std::hash<unsigned int>{}(axis);
    rview_hash ^= std::hash<double>{}(zpos);
  }

  // If the parameters changed, we need to send the image to the server
  if(rview_hash != m_RemoteImageViewHash)
  {
    ProgressTaskGuard task_tracker(m_ProgressDelegate, "Uploading image to DLS", true);

    // Keep track of elapsed time
    Clock::time_point t0, t1, t2, t3, t4, t5;
    t0 = Clock::now();

    // Create a Mime packet to send to the server
    RESTMultipartData mpd;
    std::string gzip_buffer;

    // What we send to the server depends on image dimensionality
    if(model_metadata.dimensions == 3)
    {
      // Send complete image content. Check if the model accepts multi-dimensional content
      if(actual_layer->IsScalar())
      {
        auto *fsrc = actual_layer->CreateCastToFloatVectorPipeline("DLSExport");
        t1 = Clock::now();
        EncodeImage(mpd, fsrc, gzip_buffer);
        t2 = Clock::now();
      }
      else
      {
        auto *fsrc = actual_layer->CreateCastToFloatPipeline("DLSExport");
        t1 = Clock::now();
        EncodeImage(mpd, fsrc, gzip_buffer);
        t2 = Clock::now();
      }
      actual_layer->ReleaseInternalPipeline("DLSExport");
    }
    else if(model_metadata.dimensions == 2)
    {
      // We need to extract a slice along the current dimension
      auto di = DisplaySliceIndex(axis, DISPLAY_SLICE_MAIN);
      if(actual_layer->IsScalar())
      {
        auto *fsrc = actual_layer->CreateCastToFloatSlicePipeline("DLSExport", di);
        t1 = Clock::now();
        EncodeImage(mpd, fsrc, gzip_buffer);
        t2 = Clock::now();
      }
      else
      {
        auto *fsrc = actual_layer->CreateCastToFloatVectorSlicePipeline("DLSExport", di);
        t1 = Clock::now();
        EncodeImage(mpd, fsrc, gzip_buffer);
        t2 = Clock::now();
      }
      actual_layer->ReleaseInternalPipeline("DLSExport", di);
    }

    /*
    // Create a command for progress reporting
    auto *pdel = m_ParentModel->GetProgressReporterDelegate();
    pdel->Show("Uploading image to server...");
    SmartPtr<itk::Command> cmd_progress = pdel->CreateCommand();
    SmartPtr<AllPurposeProgressAccumulator> accum = AllPurposeProgressAccumulator::New();
    accum->AddObserver(itk::ProgressEvent(), cmd_progress);
    void *transfer_progress_src = accum->RegisterGenericSource(1, 1.0);

    // Write the image in NIFTI format to a temporary location
    cli.SetProgressCallback(transfer_progress_src, AllPurposeProgressAccumulator::GenericProgressCallback);
    */
    cli.SetProgressCallback(&task_tracker, &ProgressTaskGuard::ProgressCallback);

    t3 = Clock::now();
    if(!cli.PostMultipart("upload_raw/%s?filename=upload.nii.gz", &mpd, m_ActiveSession.c_str()))
    {
      throw IRISException("Error uploading image to DSL server: %s", cli.GetErrorString());
    }
    t4 = Clock::now();

    // Hide the progress reporter
    // pdel->Hide();

    // Free wasted memory
    // accum->UnregisterAllSources();

    std::cout << "Pipeline time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms "
              << std::endl;

    std::cout << "Compression time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms "
              << std::endl;

    std::cout << "Server time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count() << " ms "
              << std::endl;

    m_RemoteImageViewHash = rview_hash;
    m_LabelState = -1;
  }
}

void
DeepLearningSegmentationModel::ResetInteractions()
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  // Perform the drawing command
  RESTClientType cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());

  if (!cli.Get("reset_interactions/%s", m_ActiveSession.c_str()))
  {
    std::cerr << "RESP:" << cli.GetOutput() << std::endl;
    throw IRISException("Failed to reset interactions");
  }
  std::cout << "RESET INTERACTIONS" << std::endl;
}


bool
DeepLearningSegmentationModel::ResetInteractionsIfNeeded()
{
  // Check if the interaction state needs to be reset - if the active label has changed
  auto *gs = m_ParentModel->GetGlobalState();
  if(m_LabelState >= 0 && m_LabelState != gs->GetDrawingColorLabel())
  {
    this->ResetInteractions();
    return true;
  }
  return false;
}

bool
DeepLearningSegmentationModel::UpdateSegmentation(const std::string &model_id,
                                                  int                axis,
                                                  const char        *json,
                                                  const char        *commit_name)
{
  auto *gs = m_ParentModel->GetGlobalState();

  Json::Reader json_reader;
  Json::Value  root;
  if (json_reader.parse(json, root, false))
  {
    const Json::Value result = root["result"];

    // Current segmentation
    auto *seg = m_ParentModel->GetDriver()->GetSelectedSegmentationLayer();

    // Expected number of characters to receive
    // TODO: this is wrong for 2D
    size_t expected_size = seg->GetImageBase()->GetBufferedRegion().GetNumberOfPixels();

    // Decode the result
    std::string result_b64 = result.asString();
    std::cout << "Segmentation payload size: " << result_b64.size() << std::endl;
    std::string result_gzip = base64::from_base64(result_b64);
    std::cout << "After base64 decoding: " << result_gzip.size() << std::endl;
    std::cout << "  " << (int)((unsigned char)result_gzip[0]) << ", "
              << (int)((unsigned char)result_gzip[1]) << std::endl;
    // std::string result_raw = gzip_decompress(result_gzip, expected_size);
    std::string result_raw;
    auto        t2 = Clock::now();
    result_raw.reserve(expected_size);
    if (!gzipInflate(result_gzip, result_raw))
      throw IRISException("Error decompressing gzipped payload");
    auto t3 = Clock::now();

    std::cout << "Gzip time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms "
              << std::endl;

    std::cout << "After gzip decoding: " << result_raw.size() << std::endl;
    std::cout << "Expected: " << expected_size << std::endl;

    // Save the current label state
    m_LabelState = gs->GetDrawingColorLabel();

    auto &model_metadata = GetRemoteModelMetadata(model_id);
    if(model_metadata.dimensions == 3)
    {
      // Create an ITK image of the segmentation for this label
      using ImageType = itk::Image<char, 3>;
      ImageType::Pointer result_img = ImageType::New();
      result_img->CopyInformation(seg->GetImageBase());
      result_img->SetRegions(seg->GetImageBase()->GetBufferedRegion());
      result_img->GetPixelContainer()->SetImportPointer(result_raw.data(), result_raw.size(), false);

      // The result is floating point values converted to a string
      SegmentationUpdateIterator itVol(seg,
                                       seg->GetImageBase()->GetBufferedRegion(),
                                       gs->GetDrawingColorLabel(),
                                       gs->GetDrawOverFilter());
      itk::ImageRegionIterator<ImageType> itSrc(result_img, seg->GetImageBase()->GetBufferedRegion());

      for (; !itVol.IsAtEnd(); ++itVol, ++itSrc)
      {
        if (itSrc.Get() > 0.0)
          itVol.PaintAsForeground();
        else
          itVol.PaintAsBackground();
      }
      if (itVol.Finalize(commit_name))
      {
        m_ParentModel->GetDriver()->RecordCurrentLabelUse();
        m_ParentModel->GetDriver()->InvokeEvent(SegmentationChangeEvent());
        return true;
      }
    }
    else
    {
      // Determine the size of the region
      auto di = DisplaySliceIndex(axis, DISPLAY_SLICE_MAIN);
      auto *seg_slice = seg->GetSlice(di);

      // TODO: seems wasteful just to get region information
      seg_slice->Update();

      // Create an ITK image of the segmentation for this label
      using ImageType = itk::Image<unsigned char, 2>;
      ImageType::Pointer result_img = ImageType::New();
      result_img->CopyInformation(seg_slice);
      result_img->SetRegions(seg_slice->GetBufferedRegion());

      // Check that the size of raw data matches the buffer size
      if(result_img->GetBufferedRegion().GetNumberOfPixels() != result_raw.size())
        throw IRISException("DLS server returned segmentation that does not match the size of the image");
      result_img->GetPixelContainer()->SetImportPointer((unsigned char *) result_raw.data(), result_raw.size(), false);

      // Get the position of the current slice
      auto xSlice = seg->MapImageCIndexToSliceCIndex(
        axis, to_double(m_ParentModel->GetDriver()->GetCursorPosition()));

      // Update the global segmentation
      m_ParentModel->GetDriver()->UpdateSegmentationWithSliceDrawing(
        result_img,
        seg->GetImageGeometry()->GetDisplayToImageTransform(axis),
        xSlice[2], false,
        "DLS result");
    }
  }
  return false;
}

std::string
DeepLearningSegmentationModel::GetActualServerURL()
{
  if(m_ServerIndex < 0)
    return std::string();

  auto sp = m_ServerProperties[m_ServerIndex];

  if (!sp->GetRemoteConnection())
  {
    if (m_LocalPortNumber >= 0)
    {
      std::ostringstream oss;
      oss << "http://localhost:";
      oss << m_LocalPortNumber;
      return oss.str();
    }
    else
      return std::string();
  }

  if(sp->GetUseSSHTunnel())
    return GetProxyURL();
  else
    return sp->GetFullURL();
}


bool
DeepLearningSegmentationModel::PerformPointInteraction(std::string       model_id,
                                                       ImageWrapperBase *layer,
                                                       int               axis,
                                                       Vector3ui         pos,
                                                       bool              reverse)
{
  // Update the source image
  this->SetSourceImage(model_id, layer, axis);

  // Reset interactions if needed
  this->ResetInteractionsIfNeeded();

  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  // Set waiting cursor for DLS operation
  QtCursorOverride c(Qt::WaitCursor);
  
  // Perform the drawing command
  Clock::time_point t0, t1, t2, t3;
  t0 = Clock::now();
  RESTClientType cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());

  // Start a task tracker
  ProgressTaskGuard task_tracker(m_ProgressDelegate, "Performing point interaction");
  cli.SetProgressCallback(&task_tracker, &ProgressTaskGuard::ProgressCallback);

  // What's the dimensionality of our model?
  auto &model_metadata = GetRemoteModelMetadata(model_id);
  bool rc;
  if(model_metadata.dimensions == 3)
  {
    rc = cli.Get("v2/process_point_interaction/%s?point=%d&point=%d&point=%d&foreground=%s",
                 m_ActiveSession.c_str(),
                 pos[0],
                 pos[1],
                 pos[2],
                 reverse ? "false" : "true");
  }
  else
  {
    // TODO: this is the wrong calculation!
    Vector3i spos = to_int(layer->MapImageCIndexToSliceCIndex(axis, to_double(pos) + Vector3d(0.5)));
    rc = cli.Get("v2/process_point_interaction/%s?point=%d&point=%d&foreground=%s",
                 m_ActiveSession.c_str(),
                 spos[0],
                 spos[1],
                 reverse ? "false" : "true");
  }
  cli.RemoveProgressCallback();
  std::cout << "*** COMPLETED POINT INTERACTION ***" << std::endl;

  if(!rc)
  {
    std::cerr << "RESP:" << cli.GetOutput() << std::endl;
    throw IRISException("Failed to send current coordinate to the server");
  }
  t1 = Clock::now();
  std::cout << "Interaction time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms "
            << std::endl;

  return this->UpdateSegmentation(model_id, axis, cli.GetOutput(), "nnInteractive point interaction");
}

bool
DeepLearningSegmentationModel::PerformScribbleOrLassoInteraction(const char        *target_url,
                                                                 std::string        model_id,
                                                                 ImageWrapperBase  *layer,
                                                                 int                axis,
                                                                 LabelImageWrapper *seg,
                                                                 bool               reverse)
{
  // Update the source image
  this->SetSourceImage(model_id, layer, axis);

  // Reset interactions if needed
  this->ResetInteractionsIfNeeded();

  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  // Set waiting cursor for DLS operation
  QtCursorOverride c(Qt::WaitCursor);

  // Create a multipart dataset with the segmentation
  RESTMultipartData mpd;
  std::string       gzip_buffer;

  // TODO: this is a ridiculous waste of time, expanding a small RLE image to then compress
  // it with gzip. Instead, implement RLE on the server.
  auto t0 = Clock::now();
  using FloatImageType = typename ImageWrapperBase::FloatImageType;
  FloatImageType *src = seg->CreateCastToFloatPipeline("DLSExport");
  src->Update();
  EncodeImage(mpd, src, gzip_buffer);
  auto t1 = Clock::now();

  // Perform the drawing command
  auto       t2 = Clock::now();
  RESTClientType cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());
  if (!cli.PostMultipart(
        "%s/%s?foreground=%s", &mpd, target_url, m_ActiveSession.c_str(), reverse ? "false" : "true"))
  {
    std::cerr << "RESP:" << cli.GetOutput() << std::endl;
    throw IRISException("Failed to send current coordinate to the server");
  }
  auto t3 = Clock::now();
  std::cout << "Encoding time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms "
            << std::endl;
  std::cout << "Interaction time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms "
            << std::endl;

  return this->UpdateSegmentation(model_id, axis, cli.GetOutput(), "nnInteractive scribble interaction");
}

const dls_model::RemoteModelMetadata &
DeepLearningSegmentationModel::GetRemoteModelMetadata(const std::string &model_id)
{
  auto it_mm = m_RemoteModelMetadata.find(model_id);
  if(it_mm == m_RemoteModelMetadata.end())
    throw IRISException("Requested model %s is not present on the server", model_id.c_str());
  return it_mm->second;
}

void
DeepLearningServerPropertiesModel::AddKnownLocalPythonExePath(const std::string exepath)
{
  m_LocalPythonExeDomain[exepath] = exepath;
  if(m_LocalPythonExePathInternalModel->GetValue().empty())
    m_LocalPythonExePathInternalModel->SetValue(exepath);

  m_LocalPythonExePathModel->InvokeEvent(DomainChangedEvent());
  m_LocalPythonExePathModel->InvokeEvent(ValueChangedEvent());
}

bool
DeepLearningServerPropertiesModel::GetFullURLValue(std::string &value)
{
  if(GetHostname().size() > 0 && GetPort() > 0)
  {
    std::string method = GetPort() == 443 ? "https" : "http";
    char buffer[256];
    snprintf(buffer, 256, "%s://%s:%d", method.c_str(), GetHostname().c_str(), GetPort());
    value = buffer;
    return true;
  }
  else
  {
    value = "";
    return false;
  }
}

bool
DeepLearningServerPropertiesModel::GetDisplayNameValue(std::string &value)
{
  if(GetNickname().size())
  {
    value = GetNickname();
  }
  else if(GetRemoteConnection())
  {
    value = GetFullURL();
  }
  else
  {
    value = GetLocalPythonVEnvPath();
  }
  return true;
}

bool
DeepLearningServerPropertiesModel::GetLocalPythonExePathValueAndRange(std::string     &value,
                                                                      PythonExeDomain *domain)
{
  bool retval = m_LocalPythonExePathInternalModel->GetValueAndDomain(value, nullptr);
  if(!retval || value.empty())
    return false;

  // Make sure that the current value is part of the domain
  if(m_LocalPythonExeDomain.find(value) == m_LocalPythonExeDomain.end())
  {
    m_LocalPythonExeDomain[value] = value;
    m_LocalPythonExePathModel->InvokeEvent(DomainChangedEvent());
  }

  if(domain)
    *domain = m_LocalPythonExeDomain;

  return retval;
}

void
DeepLearningServerPropertiesModel::SetLocalPythonExePathValue(std::string value)
{
  m_LocalPythonExePathInternalModel->SetValue(value);
  if(!value.empty())
  {
    m_LocalPythonExeDomain[value] = value;
    m_LocalPythonExePathModel->InvokeEvent(DomainChangedEvent());
  }
}


string
DeepLearningServerPropertiesModel::GetHash() const
{
  std::ostringstream oss;

  // Complete code below to use all fields in this class
  oss << this->GetHostname() << this->GetPort() << this->GetUseSSHTunnel()
      << this->GetSSHUsername() << this->GetSSHPrivateKeyFile() << this->GetRemoteConnection()
      << this->GetLocalPythonVEnvPath();

  char hex_code[33];
  hex_code[32] = 0;
  itksysMD5 *md5 = itksysMD5_New();
  itksysMD5_Initialize(md5);
  itksysMD5_Append(md5, (unsigned char *) oss.str().c_str(), oss.str().size());
  itksysMD5_FinalizeHex(md5, hex_code);
  itksysMD5_Delete(md5);

  return std::string(hex_code);
}

DeepLearningServerPropertiesModel::DeepLearningServerPropertiesModel()
{
  m_NicknameModel = NewSimpleProperty("Nickname", std::string());
  m_RemoteConnectionModel = NewSimpleProperty("RemoteConnection", false);
  m_HostnameModel = NewSimpleProperty("Hostname", std::string());
  m_PortModel = NewSimpleProperty("Port", 8911);
  m_UseSSHTunnelModel = NewSimpleProperty("UseSSHTunnel", false);
  m_SSHUsernameModel = NewSimpleProperty("SSHUsername", std::string());
  m_SSHPrivateKeyFileModel = NewSimpleProperty("SSHPrivateKeyFile", std::string());
  m_LocalPythonExePathInternalModel = NewSimpleProperty("LocalPythonExePath", std::string());
  m_LocalPythonVEnvPathModel = NewSimpleProperty("LocalPythonVEnvPath", std::string());
  m_NoSSLVerifyModel = NewSimpleProperty("NoSSLVerify", false);

  m_FullURLModel = wrapGetterSetterPairAsProperty(this, &Self::GetFullURLValue);  
  m_FullURLModel->RebroadcastFromSourceProperty(m_HostnameModel);
  m_FullURLModel->RebroadcastFromSourceProperty(m_PortModel);

  m_DisplayNameModel = wrapGetterSetterPairAsProperty(this, &Self::GetDisplayNameValue);
  m_DisplayNameModel->RebroadcastFromSourceProperty(m_HostnameModel);
  m_DisplayNameModel->RebroadcastFromSourceProperty(m_PortModel);
  m_DisplayNameModel->RebroadcastFromSourceProperty(m_RemoteConnectionModel);
  m_DisplayNameModel->RebroadcastFromSourceProperty(m_NicknameModel);
  m_DisplayNameModel->RebroadcastFromSourceProperty(m_LocalPythonVEnvPathModel);

  m_LocalPythonExePathModel = wrapGetterSetterPairAsProperty(
    this, &Self::GetLocalPythonExePathValueAndRange, &Self::SetLocalPythonExePathValue);
  m_LocalPythonExePathModel->RebroadcastFromSourceProperty(m_LocalPythonExePathInternalModel);
}
