#ifndef IMAGEMECHANICSTRAITS_H
#define IMAGEMECHANICSTRAITS_H


/**
 * Common base class for the traits, implements helper functions
 * that are shared by the traits classes
 */
template <class TImage> class ImageMechanicsTraitsBase
{
public:


};


/**
 * A traits class that can be used to treat Image, VectorImage, RLEImage,
 * ImageAdaptor, etc. in a transparent way. It provides static functions
 * that can be used to increase/decrease image dimensions, access pixels,
 * etc.
 */
template <class TImage> class ImageMechanicsTraits
    : public ImageMechanicsTraitsBase
{
public:
  static constexpr unsigned int ImageDimension = TImage::ImageDimension;

  /** Increase the image dimension */

};

#endif // IMAGEMECHANICSTRAITS_H
