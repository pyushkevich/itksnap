#include "SNAPQtCommonTranslations.h"

QString
SNAPQtCommonTranslations::translate(ImageIODisplayName name)
{
  switch (name)
  {
    case ImageIODisplayName::MAIN:
      return tr("Main Image");
    case ImageIODisplayName::SEGMENTATION:
      return tr("Segmentation Image");
    case ImageIODisplayName::OVERLAY:
      return tr("Additional Image");
    case ImageIODisplayName::ANATOMICAL:
      return tr("Image");
    case ImageIODisplayName::SPEED:
      return tr("Speed Image");
    case ImageIODisplayName::LEVELSET:
      return tr("Level Set Image");
    case ImageIODisplayName::CLASSIFIER_SAMPLES:
      return tr("Classifier Samples Image");
  }
}

QString
SNAPQtCommonTranslations::translate(PreprocessingMode mode)
{
  switch (mode)
  {
    case PREPROCESS_NONE:
      return tr("None");
    case PREPROCESS_THRESHOLD:
      return tr("Thresholding");
    case PREPROCESS_RF:
      return tr("Classification");
    case PREPROCESS_GMM:
      return tr("Clustering");
    case PREPROCESS_EDGE:
      return tr("Edge Attraction");
  }
}
