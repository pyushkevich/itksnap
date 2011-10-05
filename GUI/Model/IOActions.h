#ifndef IOACTIONS_H
#define IOACTIONS_H

#include "UIAction.h"
#include "SNAPCommon.h"
#include "IRISApplication.h"
#include <string>

class GuidedNativeImageIO;

class LoadMainImageAction : public UIAbstractAction
{
public:
  irisITKObjectMacro(LoadMainImageAction, UIAbstractAction)

  void Initialize(GlobalUIModel *model,
                  const char *file,
                  IRISApplication::MainImageType type);


  void Execute();

protected:
  std::string m_File;
  IRISApplication::MainImageType m_ImageType;

  LoadMainImageAction() {}
  ~LoadMainImageAction() {}
};


class LoadSegmentationAction : public UIAbstractAction
{
public:
  irisITKObjectMacro(LoadSegmentationAction, UIAbstractAction)

  void Initialize(GlobalUIModel *model,
                  const char *file);

  void Execute();

protected:
  std::string m_File;
  IRISApplication::MainImageType m_ImageType;

  LoadSegmentationAction() {}
  ~LoadSegmentationAction() {}
};



class UnloadMainImageAction : public UIAbstractAction
{
public:
  irisITKObjectMacro(UnloadMainImageAction, UIAbstractAction)

  void Execute();

protected:
  std::string m_File;

  UnloadMainImageAction() {}
  ~UnloadMainImageAction() {}

};









class AbstractLoadImageDelegate
{
public:

  AbstractLoadImageDelegate(GlobalUIModel *model)
    { m_Model = model; }

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void UnloadCurrentImage() = 0;
  virtual void UpdateApplicationWithImage(GuidedNativeImageIO *io) = 0;

protected:
  GlobalUIModel *m_Model;
};

class LoadMainImageDelegate : public AbstractLoadImageDelegate
{
public:

  LoadMainImageDelegate(GlobalUIModel *model,
                        IRISApplication::MainImageType type);

  void UnloadCurrentImage();
  void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl);
  void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);

protected:
  IRISApplication::MainImageType m_ImageType;
};

class LoadOverlayImageDelegate : public AbstractLoadImageDelegate
{
public:

  LoadOverlayImageDelegate(GlobalUIModel *model,
                        IRISApplication::MainImageType type);

  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);

protected:
  IRISApplication::MainImageType m_ImageType;
};

class LoadSegmentationImageDelegate : public AbstractLoadImageDelegate
{
public:

  LoadSegmentationImageDelegate(GlobalUIModel *model);

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);
  virtual void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl);
  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);

protected:

};



#endif // IOACTIONS_H
