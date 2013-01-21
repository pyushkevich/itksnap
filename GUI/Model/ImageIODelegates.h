#ifndef IMAGEIODELEGATES_H
#define IMAGEIODELEGATES_H

#include "SNAPCommon.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISException.h"
#include <vector>

class GlobalUIModel;
class IRISWarningList;

class IRISWarningList : public std::vector<IRISWarning> {};

/**
  A parent class for all IO delegates. Child objects must implement the
  virtual members defined in this class. Delegates are used to specialize
  the behavior of image IO wizards, as well as to load images from
  command line.
  */
class AbstractLoadImageDelegate
{
public:

  AbstractLoadImageDelegate(GlobalUIModel *model)
    { m_Model = model; }

  virtual ~AbstractLoadImageDelegate() {}

  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void ValidateImage(GuidedNativeImageIO *io, IRISWarningList &wl) {}
  virtual void UnloadCurrentImage() = 0;
  virtual void UpdateApplicationWithImage(GuidedNativeImageIO *io) = 0;

protected:
  GlobalUIModel *m_Model;
};

class LoadAnatomicImageDelegate : public AbstractLoadImageDelegate
{
public:

  LoadAnatomicImageDelegate(GlobalUIModel *model)
    : AbstractLoadImageDelegate(model) {}

  virtual ~LoadAnatomicImageDelegate() {}
  virtual void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);

};

class LoadMainImageDelegate : public LoadAnatomicImageDelegate
{
public:

  LoadMainImageDelegate(GlobalUIModel *model);

  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);

protected:

};

class LoadOverlayImageDelegate : public LoadAnatomicImageDelegate
{
public:

  LoadOverlayImageDelegate(GlobalUIModel *model);

  void UnloadCurrentImage();
  void UpdateApplicationWithImage(GuidedNativeImageIO *io);
  void ValidateHeader(GuidedNativeImageIO *io, IRISWarningList &wl);
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


#endif // IMAGEIODELEGATES_H
