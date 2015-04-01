#ifndef IMAGEIODELEGATES_H
#define IMAGEIODELEGATES_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include <vector>

class IRISApplication;
class IRISWarningList;
class ImageWrapperBase;

class IRISWarningList : public std::vector<IRISWarning> {};

/**
  A parent class for all IO delegates. Child objects must implement the
  virtual members defined in this class. Delegates are used to specialize
  the behavior of image IO wizards, as well as to load images from
  command line.
  */
class AbstractLoadImageDelegate : public itk::Object
{
public:

  irisITKAbstractObjectMacro(AbstractLoadImageDelegate, itk::Object)

  virtual void Initialize(IRISApplication *driver)
    { m_Driver = driver; }

  /**
   * Set the registry that will be used to load image-level and project-level
   * metadata associated with the image. If the registry is not specified, the
   * code will attempt to load it from the automatically created image
   * association files (created every time an image is closed). The registry
   * should contain a folder LayerMetaData for the layer-specific stuff (e.g.,
   * colormap, etc). For layers in the "MAIN Image" role, it can contain a
   * folder "ProjectMetaData" as well.
   */
  irisGetSetMacro(MetaDataRegistry, Registry *)

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void UnloadCurrentImage() = 0;
  virtual void UpdateApplicationWithImage(GuidedNativeImageIO *io) = 0;

  virtual bool GetUseRegistration() const { return false; }
  virtual bool IsOverlay() const { return false; }

protected:
  AbstractLoadImageDelegate() : m_MetaDataRegistry(NULL) {}
  virtual ~AbstractLoadImageDelegate() {}

  IRISApplication *m_Driver;

private:
  Registry *m_MetaDataRegistry;
};

class LoadAnatomicImageDelegate : public AbstractLoadImageDelegate
{
public:

  irisITKAbstractObjectMacro(LoadAnatomicImageDelegate, AbstractLoadImageDelegate)

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);

protected:
  LoadAnatomicImageDelegate() {}
  virtual ~LoadAnatomicImageDelegate() {}
};

class LoadMainImageDelegate : public LoadAnatomicImageDelegate
{
public:

  irisITKObjectMacro(LoadMainImageDelegate, LoadAnatomicImageDelegate)

  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);

protected:
  LoadMainImageDelegate() {}
  virtual ~LoadMainImageDelegate() {}

};

class LoadOverlayImageDelegate : public LoadAnatomicImageDelegate
{
public:

  irisITKObjectMacro(LoadOverlayImageDelegate, LoadAnatomicImageDelegate)

  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);
  void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);
  virtual bool IsOverlay() const { return true; }

protected:
  LoadOverlayImageDelegate() { }
  virtual ~LoadOverlayImageDelegate() {}
};


class LoadCoregisteredOverlayImageDelegate : public LoadAnatomicImageDelegate
{
public:

  irisITKObjectMacro(LoadCoregisteredOverlayImageDelegate, LoadAnatomicImageDelegate)

  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);
  void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);

  virtual bool GetUseRegistration() const { return true; }
  virtual bool IsOverlay() const { return true; }


protected:
  LoadCoregisteredOverlayImageDelegate() { }
  virtual ~LoadCoregisteredOverlayImageDelegate() {}
};



class LoadSegmentationImageDelegate : public AbstractLoadImageDelegate
{
public:

  irisITKObjectMacro(LoadSegmentationImageDelegate, AbstractLoadImageDelegate)

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);
  virtual void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl);
  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);

protected:
  LoadSegmentationImageDelegate() {}
  virtual ~LoadSegmentationImageDelegate() {}
};


class AbstractSaveImageDelegate : public itk::Object
{
public:

  irisITKAbstractObjectMacro(AbstractSaveImageDelegate, itk::Object)

  virtual void Initialize(IRISApplication *driver)
    { m_Driver = driver; m_SaveSuccessful = false; }

  virtual void SaveImage(
      const std::string &fname,
      GuidedNativeImageIO *io,
      Registry &reg,
      IRISWarningList &wl) = 0;

  virtual const char *GetCurrentFilename() = 0;

  virtual const char *GetHistoryName() = 0;

  /**
   * Has the file been saves successfully after the call to SaveImage?
   */
  bool IsSaveSuccessful() { return m_SaveSuccessful; }

protected:
  AbstractSaveImageDelegate() {}
  virtual ~AbstractSaveImageDelegate() {}

  IRISApplication *m_Driver;
  bool m_SaveSuccessful;
};

class DefaultSaveImageDelegate : public AbstractSaveImageDelegate
{
public:
  irisITKObjectMacro(DefaultSaveImageDelegate, AbstractSaveImageDelegate)

  virtual void Initialize(IRISApplication *driver,
                          ImageWrapperBase *wrapper,
                          const std::string &histname,
                          bool trackInLocalHistory = true);

  // Add a history name to update when the filename is saved. It is possible
  // for multiple history names to be updated
  void AddHistoryName(const std::string &histname);

  virtual const char *GetHistoryName();

  virtual void SaveImage(
      const std::string &fname,
      GuidedNativeImageIO *io,
      Registry &reg,
      IRISWarningList &wl);

  virtual const char *GetCurrentFilename();

protected:

  DefaultSaveImageDelegate() {}
  virtual ~DefaultSaveImageDelegate() {}

  ImageWrapperBase *m_Wrapper;
  std::list<std::string> m_HistoryNames;
  bool m_Track;
};

#endif // IMAGEIODELEGATES_H
