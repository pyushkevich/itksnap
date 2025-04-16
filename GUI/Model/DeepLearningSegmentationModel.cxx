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
#include <chrono>
#include "itksys/MD5.h"

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
  else if (ssh_userauth_password(session, nullptr, password.c_str()) != SSH_AUTH_SUCCESS)
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

  // Server status model
  m_ServerStatusModel = NewSimpleConcreteProperty(dls_model::ConnectionStatus());
  m_ServerProgressModel = NewRangedConcreteProperty(0., 0., 0., 0.);

  // Set up the progress command
  m_ProgressCommand = itk::MemberCommand<Self>::New();
  m_ProgressCommand->SetCallbackFunction(this, &Self::ProgressCallback);

  this->Rebroadcast(m_ServerModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerModel, DomainChangedEvent(), ServerChangeEvent());

  // Initialize the cookie jar
  m_RESTSharedData = new RESTSharedData();
}

DeepLearningSegmentationModel::~DeepLearningSegmentationModel()
{
  delete m_RESTSharedData;
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
DeepLearningSegmentationModel::SetServerValue(int value)
{
  // Check if the value is in range
  itkAssertOrThrowMacro(value == -1 || (value >= 0 && value < m_ServerProperties.size()),
                        "Incorrect value in DeepLearningSegmentationModel::SetServerValue");

  // Set the current server index
  m_ServerIndex = value;

  // Reset the session variables - we are starting over
  m_ActiveSession = std::string();
  m_ActiveLayer = std::make_tuple(0u, 0u);

  // Reset the proxy URL
  SetProxyURL(std::string());

  // Reset the connection status to not connected state
  SetServerStatus(value >= 0 ? dls_model::ConnectionStatus(dls_model::CONN_NO_SERVER)
                             : dls_model::ConnectionStatus(dls_model::CONN_NOT_CONNECTED));

  // Fire some events
  m_ServerModel->InvokeEvent(ValueChangedEvent());
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

DeepLearningSegmentationModel::StatusCheck
DeepLearningSegmentationModel::AsyncCheckStatus()
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  dls_model::ConnectionStatus response;

  // Check if URL has been set
  if (m_ServerIndex < 0)
  {
    response.status = dls_model::CONN_NO_SERVER;
    return std::make_pair(std::string(), response);
  }

  // Check if the proxy has been set up for an SSH connection
  auto sp = m_ServerProperties[m_ServerIndex];
  if (sp->GetUseSSHTunnel() && GetProxyURL().size() == 0)
  {
    response.status = dls_model::CONN_NOT_CONNECTED;
    response.error_message = "SSH tunnel has not been configured";
  }
  else
  {
    try
    {
      RESTClient cli(m_RESTSharedData);
      cli.SetServerURL(GetActualServerURL().c_str());
      bool status = cli.Get("status");
      if (!status)
      {
        response.status = dls_model::CONN_NOT_CONNECTED;
        response.error_message = cli.GetErrorString();
      }
      else
      {
        Json::Reader json_reader;
        Json::Value  root;
        if (json_reader.parse(cli.GetOutput(), root, false))
        {
          response.status = dls_model::CONN_CONNECTED;
          response.server_version = root["version"].asString();
        }
        else
          throw IRISException("Server did not provide version number");
      }
    }
    catch (IRISException &exc)
    {
      response.status = dls_model::CONN_NOT_CONNECTED;
      response.error_message = exc.what();
    }
  }

  return std::make_pair(sp->GetHash(), response);
}

void
DeepLearningSegmentationModel::ApplyStatusCheckResponse(const StatusCheck &result)
{
  std::string hash = m_ServerIndex < 0 ? std::string() : m_ServerProperties[m_ServerIndex]->GetHash();
  if(hash == result.first)
    this->SetServerStatus(result.second);
}

void
DeepLearningSegmentationModel::SetSourceImage(ImageWrapperBase *layer)
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  RESTClient cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());

  // Check if we have an open session with the server, if not establish it
  if(m_ActiveSession.size() == 0)
  {
    Clock::time_point t0, t1;
    t0 = Clock::now();
    if(!cli.Get("start_session"))
      throw IRISException("Error creating session on DLS server: %s", cli.GetErrorString());
    t1 = Clock::now();

    Json::Reader json_reader;
    Json::Value root;
    if(json_reader.parse(cli.GetOutput(), root, false))
    {
      const Json::Value result = root["session_id"];
      m_ActiveSession = result.asString();
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
  LayerSelection sel = std::make_tuple(layer->GetUniqueId(), tp);
  if(m_ActiveLayer != sel)
  {
    Clock::time_point t0, t1, t2, t3, t4, t5;

    // Export the current image to a file that can be uploaded
    t0 = Clock::now();
    auto *id = driver->GetCurrentImageData();
    using FloatImageType = typename ImageWrapperBase::FloatImageType;
    FloatImageType *src = layer->GetDefaultScalarRepresentation()->CreateCastToFloatPipeline("DLSExport");
    src->Update();
    t1 = Clock::now();

    // Create a Mime packet
    RESTMultipartData mpd;
    std::string gzip_buffer;
    t2 = Clock::now();
    EncodeImage(mpd, src, gzip_buffer);
    t3 = Clock::now();

    // Create a command for progress reporting
    auto *pdel = m_ParentModel->GetProgressReporterDelegate();
    pdel->Show("Uploading image to server...");
    SmartPtr<itk::Command> cmd_progress = pdel->CreateCommand();
    SmartPtr<AllPurposeProgressAccumulator> accum = AllPurposeProgressAccumulator::New();
    accum->AddObserver(itk::ProgressEvent(), cmd_progress);
    void *transfer_progress_src = accum->RegisterGenericSource(1, 1.0);

    // Write the image in NIFTI format to a temporary location
    cli.SetProgressCallback(transfer_progress_src, AllPurposeProgressAccumulator::GenericProgressCallback);

    t4 = Clock::now();
    if(!cli.PostMultipart("upload_raw/%s?filename=upload.nii.gz", &mpd, m_ActiveSession.c_str()))
    {
      throw IRISException("Error uploading image to DSL server: %s", cli.GetErrorString());
    }
    t5 = Clock::now();

    // Hide the progress reporter
    pdel->Hide();

    // Free wasted memory
    layer->ReleaseInternalPipeline("DLSExport");
    accum->UnregisterAllSources();

    std::cout << "Pipeline time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms "
              << std::endl;

    std::cout << "Compression time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms "
              << std::endl;

    std::cout << "Server time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count() << " ms "
              << std::endl;

    m_ActiveLayer = sel;
    m_LabelState = -1;
    // pdel->Hide();
  }
}

void
DeepLearningSegmentationModel::ResetInteractions()
{
  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

  // Perform the drawing command
  RESTClient cli(m_RESTSharedData);
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
DeepLearningSegmentationModel::UpdateSegmentation(const char *json, const char *commit_name)
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

    // Create an ITK image of the segmentation for this label
    using ImageType = itk::Image<char, 3>;
    ImageType::Pointer result_img = ImageType::New();
    result_img->CopyInformation(seg->GetImageBase());
    result_img->SetRegions(seg->GetImageBase()->GetBufferedRegion());
    result_img->GetPixelContainer()->SetImportPointer(result_raw.data(), result_raw.size(), false);

    // Merge this result with the segmentation
    m_LabelState = gs->GetDrawingColorLabel();

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
  return false;
}

std::string
DeepLearningSegmentationModel::GetActualServerURL()
{
  if(m_ServerIndex < 0)
    return std::string();

  auto sp = m_ServerProperties[m_ServerIndex];
  if(sp->GetUseSSHTunnel())
    return GetProxyURL();
  else
    return sp->GetFullURL();
}


bool
DeepLearningSegmentationModel::PerformPointInteraction(ImageWrapperBase *layer, Vector3ui pos, bool reverse)
{
  // Update the source image
  this->SetSourceImage(layer);

  // Reset interactions if needed
  this->ResetInteractionsIfNeeded();

  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

         // Perform the drawing command
  Clock::time_point t0, t1, t2, t3;
  t0 = Clock::now();
  RESTClient cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());
  if(!cli.Get("process_point_interaction/%s?x=%d&y=%d&z=%d&foreground=%s",
               m_ActiveSession.c_str(),
               pos[0], pos[1], pos[2],
               reverse ? "false" : "true"))
  {
    std::cerr << "RESP:" << cli.GetOutput() << std::endl;
    throw IRISException("Failed to send current coordinate to the server");
  }
  t1 = Clock::now();
  std::cout << "Interaction time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms "
            << std::endl;

  return this->UpdateSegmentation(cli.GetOutput(), "nnInteractive point interaction");
}

bool
DeepLearningSegmentationModel::PerformScribbleInteraction(ImageWrapperBase  *layer,
                                                          LabelImageWrapper *seg,
                                                          bool               reverse)
{
  // Update the source image
  this->SetSourceImage(layer);

  // Reset interactions if needed
  this->ResetInteractionsIfNeeded();

  std::lock_guard<std::mutex> guard(m_Mutex); // Prevent two threads doing IO at once

         // Create a multipart dataset with the segmentation
  RESTMultipartData mpd;
  std::string gzip_buffer;

  // TODO: this is a ridiculous waste of time, expanding a small RLE image to then compress
  // it with gzip. Instead, implement RLE on the server.
  auto t0 = Clock::now();
  using FloatImageType = typename ImageWrapperBase::FloatImageType;
  FloatImageType *src = seg->CreateCastToFloatPipeline("DLSExport");
  src->Update();
  EncodeImage(mpd, src, gzip_buffer);
  auto t1 = Clock::now();

  // Perform the drawing command
  auto t2 = Clock::now();
  RESTClient cli(m_RESTSharedData);
  cli.SetServerURL(GetActualServerURL().c_str());
  if (!cli.PostMultipart("process_scribble_interaction/%s?foreground=%s",
                         &mpd,
                         m_ActiveSession.c_str(),
                         reverse ? "false" : "true"))
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

  return this->UpdateSegmentation(cli.GetOutput(), "nnInteractive scribble interaction");
}

bool
DeepLearningServerPropertiesModel::GetFullURLValue(std::string &value)
{
  if(GetHostname().size() > 0 && GetPort() > 0)
  {
    char buffer[256];
    snprintf(buffer, 256, "http://%s:%d", GetHostname().c_str(), GetPort());
    value = buffer;
    return true;
  }
  else
  {
    value = "";
    return false;
  }
}

string
DeepLearningServerPropertiesModel::GetHash() const
{
  std::ostringstream oss;
  oss << this->GetHostname()
      << this->GetPort()
      << this->GetUseSSHTunnel()
      << this->GetSSHUsername()
      << this->GetSSHPrivateKeyFile();

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
  m_HostnameModel = NewSimpleProperty("Hostname", std::string());
  m_NicknameModel = NewSimpleProperty("Nickname", std::string());
  m_PortModel = NewSimpleProperty("Port", 8911);
  m_UseSSHTunnelModel = NewSimpleProperty("UseSSHTunnel", false);
  m_SSHUsernameModel = NewSimpleProperty("SSHUsername", std::string());
  m_SSHPrivateKeyFileModel = NewSimpleProperty("SSHPrivateKeyFile", std::string());

  m_FullURLModel = wrapGetterSetterPairAsProperty(this, &Self::GetFullURLValue);  
  m_FullURLModel->RebroadcastFromSourceProperty(m_HostnameModel);
  m_FullURLModel->RebroadcastFromSourceProperty(m_PortModel);
}
