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












#endif // IOACTIONS_H
