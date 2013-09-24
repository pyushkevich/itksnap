#ifndef IMAGEIODELEGATES_H
#define IMAGEIODELEGATES_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include <vector>

class GlobalUIModel;
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

  virtual void Initialize(GlobalUIModel *model)
    { m_Model = model; }

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void UnloadCurrentImage() = 0;
  virtual void UpdateApplicationWithImage(GuidedNativeImageIO *io) = 0;

protected:
  AbstractLoadImageDelegate() {}
  virtual ~AbstractLoadImageDelegate() {}

  GlobalUIModel *m_Model;
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

protected:
  LoadOverlayImageDelegate() {}
  virtual ~LoadOverlayImageDelegate() {}
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

  virtual void Initialize(GlobalUIModel *model)
    { m_Model = model; }

  virtual void SaveImage(
      const std::string &fname,
      GuidedNativeImageIO *io,
      Registry &reg,
      IRISWarningList &wl) = 0;

  virtual const char *GetCurrentFilename() = 0;

protected:
  AbstractSaveImageDelegate() {}
  virtual ~AbstractSaveImageDelegate() {}

  GlobalUIModel *m_Model;
};

class DefaultSaveImageDelegate : public AbstractSaveImageDelegate
{
public:
  irisITKObjectMacro(DefaultSaveImageDelegate, AbstractSaveImageDelegate)

  virtual void Initialize(GlobalUIModel *model,
                          ImageWrapperBase *wrapper,
                          const std::string &histname,
                          bool trackInLocalHistory = true);

  // Add a history name to update when the filename is saved. It is possible
  // for multiple history names to be updated
  void AddHistoryName(const std::string &histname);

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
