#include "DeepLearningSegmentationModel.h"
#include "json/json.h"
#include "GenericImageData.h"
#include "GlobalUIModel.h"
#include "PropertyModel.h"
#include "RESTClient.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include "UIReporterDelegates.h"
#include "itkImageFileWriter.h"
#include "SegmentationUpdateIterator.h"
#include "AllPurposeProgressAccumulator.h"
#include "base64.h"
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;


#if defined(ITKZLIB) && !defined(ITK_USE_SYSTEM_ZLIB)
#include "itk_zlib.h"
#else
#include "zlib.h"
#endif


bool gzipInflate( const std::string& compressedBytes, std::string& uncompressedBytes )
{
  if ( compressedBytes.size() == 0 )
  {
    uncompressedBytes = compressedBytes ;
    return true ;
  }

  uncompressedBytes.clear();
  constexpr unsigned buffer_size = 1024 * 1024;
  unsigned char buffer[buffer_size];

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
    strm.next_out = buffer;
    strm.avail_out = buffer_size;

           // strm.next_out = (Bytef *) (uncomp + strm.total_out);
           // strm.avail_out = uncompLength - strm.total_out;

           // Inflate another chunk.
    size_t n0 = strm.total_out;
    int err = inflate (&strm, Z_SYNC_FLUSH);
    size_t n = strm.total_out - n0;

           // Paste the read bytes
    uncompressedBytes.append((char *) buffer, n);

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
  unsigned char buffer[buffer_size];

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
    strm.next_out = buffer;
    strm.avail_out = buffer_size;
    size_t n0 = strm.total_out;
    int err = deflate(&strm, Z_FINISH);
    size_t n = strm.total_out - n0;

    // Paste the read bytes
    compressedBytes.append((char *) buffer, n);

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
  gzipDeflate((char *) image->GetBufferPointer(), n_bytes_raw, buffer_storage);
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


using DLSClient = RESTClient<DLSServerTraits>;


DeepLearningSegmentationModel::DeepLearningSegmentationModel()
{
  m_ServerURLModel =
    wrapGetterSetterPairAsProperty(this, &Self::GetServerURLValueAndRange, &Self::SetServerURLValue);

  m_ServerConfiguredModel = wrapGetterSetterPairAsProperty(this, &Self::GetServerConfiguredValue);
  m_ServerConfiguredModel->RebroadcastFromSourceProperty(m_ServerURLModel);

  // Server status model
  m_ServerStatusModel = NewSimpleConcreteProperty(dls_model::ConnectionStatus());
  m_ServerProgressModel = NewRangedConcreteProperty(0., 0., 0., 0.);

  // Set up the progress command
  m_ProgressCommand = itk::MemberCommand<Self>::New();
  m_ProgressCommand->SetCallbackFunction(this, &Self::ProgressCallback);

  this->Rebroadcast(m_ServerURLModel, ValueChangedEvent(), ServerChangeEvent());
  this->Rebroadcast(m_ServerURLModel, DomainChangedEvent(), ServerChangeEvent());
}

bool
DeepLearningSegmentationModel::GetServerConfiguredValue(bool &value)
{
  value = m_Server.size() > 0 && m_ServerProperties.find(m_Server) != m_ServerProperties.end();
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
DeepLearningSegmentationModel::GetServerURLValueAndRange(std::string &value, ServerURLDomain *domain)
{
  value = m_Server;
  if(domain)
    for(auto it: m_ServerProperties)
      (*domain)[it.first] = it.first;
  return true;
}

void
DeepLearningSegmentationModel::SetServerURLValue(std::string value)
{
  m_Server = value;
  const auto &sp = m_ServerProperties.find(m_Server);
  std::string url = sp == m_ServerProperties.end() ? std::string() : sp->second->GetFullURL();

  // Reset everything
  DLSClient cli;
  cli.SetServerURL(url.c_str());
  m_ActiveSession = std::string();
  m_ActiveLayer = std::make_tuple(0u, 0u);

  // Fire some events
  m_ServerURLModel->InvokeEvent(ValueChangedEvent());
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
  auto &server_folder = folder.Folder("UserServers");
  Registry::StringListType server_names;
  server_folder.GetFolderKeys(server_names);

  // Read each available server's properties
  for(auto key : server_names)
  {
    SmartPtr<DeepLearningServerPropertiesModel> spm = DeepLearningServerPropertiesModel::New();
    spm->ReadFromRegistry(server_folder.Folder(key));
    m_ServerProperties[spm->GetFullURL()] = spm;
  }

  // Read the currently selected server, make sure it is one of the servers in the list
  auto pref = folder["PreferredServer"][std::string()];
  if(m_ServerProperties.find(pref) == m_ServerProperties.end())
    pref = std::string();

  // Set the current server
  m_ServerURLModel->SetValue(pref);

  // Update everything
  m_ServerURLModel->InvokeEvent(ValueChangedEvent());
  m_ServerURLModel->InvokeEvent(DomainChangedEvent());
  m_ServerConfiguredModel->InvokeEvent(ValueChangedEvent());
}

void
DeepLearningSegmentationModel::SavePreferences(Registry &folder)
{
  // Clear the list of servers
  auto &server_folder = folder.Folder("UserServers");
  server_folder.Clear();

  // Write each server
  for(auto &it : m_ServerProperties)
  {
    it.second->WriteToRegistry(server_folder.Folder(it.second->GetFullURL()));
  }

  // Write the selected server
  folder["PreferredServer"] = m_Server;
}

DeepLearningServerPropertiesModel *
DeepLearningSegmentationModel::GetServerProperties()
{
  const auto &sp = m_ServerProperties.find(m_Server);
  return sp == m_ServerProperties.end() ? nullptr : sp->second;
}

void
DeepLearningSegmentationModel::UpdateServerProperties(DeepLearningServerPropertiesModel *model,
                                                      bool add_as_new)
{
  if(!add_as_new)
  {
    auto it = m_ServerProperties.find(m_Server);
    if(it != m_ServerProperties.end())
      m_ServerProperties.erase(it);
  }
  m_Server = model->GetFullURL();
  m_ServerProperties[m_Server] = model;
  m_ServerURLModel->InvokeEvent(ValueChangedEvent());
  m_ServerURLModel->InvokeEvent(DomainChangedEvent());
  m_ServerConfiguredModel->InvokeEvent(ValueChangedEvent());
}

void
DeepLearningSegmentationModel::DeleteCurrentServer()
{
  auto it = m_ServerProperties.find(m_Server);
  if(it != m_ServerProperties.end())
  {
    auto next_it = std::next(it);
    if(next_it == m_ServerProperties.end())
      next_it = std::prev(it);

    m_ServerProperties.erase(it);
    m_Server = next_it == m_ServerProperties.end() ? std::string() : next_it->second->GetFullURL();
    m_ServerURLModel->InvokeEvent(ValueChangedEvent());
    m_ServerURLModel->InvokeEvent(DomainChangedEvent());
    m_ServerConfiguredModel->InvokeEvent(ValueChangedEvent());
  }
}

/*
std::vector<std::string> DeepLearningSegmentationModel::GetUserServerList() const
{
  return m_ServerURLList;
}

void DeepLearningSegmentationModel::SetUserServerList(const std::vector<std::string> &servers)
{
  // Get the current server (the URL model is always valid)
  std::string my_server;
  int         curr_index = this->GetServerURL();
  if (curr_index >= 0 && curr_index < m_ServerURLList.size())
    my_server = m_ServerURLList[curr_index];

  // Reset the list of servers
  m_ServerURLList = servers;

  // Is the selected server still on the list
  if (m_ServerURLList.size() > 0)
  {
    auto it = std::find(m_ServerURLList.begin(), m_ServerURLList.end(), my_server);
    if (it == m_ServerURLList.end())
      this->SetServerURL(0);
    else
      this->SetServerURL(it - m_ServerURLList.begin());
  }
  else
  {
    this->SetServerURL(-1);
  }

  // Update the domain
  m_ServerURLModel->InvokeEvent(DomainChangedEvent());
}
*/

std::string DeepLearningSegmentationModel::GetURL(const std::string &path)
{
  // Get the main part of the URL
  auto it = m_ServerProperties.find(m_Server);
  if(it != m_ServerProperties.end())
  {
    std::string server = it->second->GetFullURL();
    return path.length() ? server + "/" + path : server;
  }
  else
  {
    return std::string();
  }
}

DeepLearningSegmentationModel::StatusCheck
DeepLearningSegmentationModel::AsyncCheckStatus(std::string url)
{
  dls_model::ConnectionStatus response;

  // Check if URL has been set
  if(!url.size())
  {
    response.status = dls_model::CONN_NO_SERVER;
    return std::make_pair(url, response);
  }

  DLSClient cli;
  try
  {
    cli.SetServerURL(url.c_str());
    bool status = cli.Get("status");
    if(!status)
    {
      response.status = dls_model::CONN_NOT_CONNECTED;
      response.error_message = cli.GetErrorString();
    }
    else
    {
      Json::Reader json_reader;
      Json::Value root;
      if(json_reader.parse(cli.GetOutput(), root, false))
      {
        response.status = dls_model::CONN_CONNECTED;
        response.server_version = root["version"].asString();
      }
      else
        throw IRISException("Server did not provide version number");
    }
  }
  catch(IRISException &exc)
  {
    response.status = dls_model::CONN_NOT_CONNECTED;
    response.error_message = exc.what();
  }

  return std::make_pair(url, response);
}

void
DeepLearningSegmentationModel::ApplyStatusCheckResponse(const StatusCheck &result)
{
  // Set the status
  if(result.first == this->GetURL(""))
  {
    this->SetServerStatus(result.second);
  }
}

void
DeepLearningSegmentationModel::SetSourceImage(ImageWrapperBase *layer)
{
  // Check if we have an open session with the server, if not establish it
  if(m_ActiveSession.size() == 0)
  {
    std::chrono::steady_clock::time_point t0, t1;
    DLSClient cli;
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
    std::chrono::steady_clock::time_point t0, t1, t2, t3, t4, t5;

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
    DLSClient cli;
    // cli.SetProgressCallback()
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
  // Perform the drawing command
  DLSClient cli;
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


bool
DeepLearningSegmentationModel::PerformPointInteraction(ImageWrapperBase *layer, Vector3ui pos, bool reverse)
{
  // Update the source image
  this->SetSourceImage(layer);

  // Reset interactions if needed
  this->ResetInteractionsIfNeeded();

  // Perform the drawing command
  std::chrono::steady_clock::time_point t0, t1, t2, t3;
  t0 = Clock::now();
  DLSClient cli;
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
  DLSClient cli;
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

DeepLearningServerPropertiesModel::DeepLearningServerPropertiesModel()
{
  m_HostnameModel = NewSimpleProperty("Hostname", std::string());
  m_NicknameModel = NewSimpleProperty("Nickname", std::string());
  m_PortModel = NewSimpleProperty("Port", 8911);
  m_UseSSHTunnelModel = NewSimpleProperty("UseSSHTunnel", false);

  m_FullURLModel = wrapGetterSetterPairAsProperty(this, &Self::GetFullURLValue);  
  m_FullURLModel->RebroadcastFromSourceProperty(m_HostnameModel);
  m_FullURLModel->RebroadcastFromSourceProperty(m_PortModel);
}
