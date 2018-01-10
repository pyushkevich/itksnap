#ifndef LABELIMAGEWRAPPER_H
#define LABELIMAGEWRAPPER_H

#include "ImageWrapperTraits.h"
#include "ScalarImageWrapper.h"

template <typename TPixel> class UndoDataManager;
template <typename TPixel> class UndoDelta;

class LabelImageWrapper : public ScalarImageWrapper<LabelImageWrapperTraits>
{
public:

  // Standard ITK business
  typedef LabelImageWrapper                                               Self;
  typedef ScalarImageWrapper<LabelImageWrapperTraits>               Superclass;
  typedef SmartPtr<Self>                                               Pointer;
  typedef SmartPtr<const Self>                                    ConstPointer;
  itkTypeMacro(LabelImageWrapper, ImageWrapper)
  itkNewMacro(Self)

  // Image Types
  typedef Superclass::ImageBaseType                              ImageBaseType;
  typedef Superclass::ImageType                                      ImageType;
  typedef Superclass::ImagePointer                                ImagePointer;
  typedef Superclass::PixelType                                      PixelType;
  typedef Superclass::ITKTransformType                        ITKTransformType;

  // Undo manager typedefs
  typedef UndoDataManager<PixelType> UndoManagerType;
  typedef UndoDelta<PixelType>       UndoManagerDelta;

  /**
   * We override the SetImage method to reset the undo manager when an image is
   * assigned to the segmentation.
   */
  virtual void UpdateImagePointer(ImageType *image,
                                  ImageBaseType *refSpace = NULL,
                                  ITKTransformType *tran = NULL) ITK_OVERRIDE;

  /**
   * Store an intermediate delta without committing it as an undo point
   * Multiple deltas can be stored and then committed with StoreUndoPoint()
   */
  void StoreIntermediateUndoDelta(UndoManagerDelta *delta);

  /**
   * Store an undo point. The first parameter is the description of the
   * update, and the second parameter is the delta to be applied. The delta
   * can be NULL. All deltas previously submitted with StoreIntermediateUndoDelta
   * and the delta passed in to this method will be commited to this undo point.
   */
  void StoreUndoPoint(const char *text, UndoManagerDelta *delta = NULL);

  /** Clear all undo points */
  void ClearUndoPoints();

  /** Check whether undo is possible */
  bool IsUndoPossible();

  /** Check whether undo is possible */
  bool IsRedoPossible();

  /** Undo (revert to last stored undo point) */
  void Undo();

  /** Redo (undo the undo) */
  void Redo();

  /** Get the undo manager */
  itkGetMacro(UndoManager, const UndoManagerType *)

  /** This is not used by the undo system itself, but uses the undo code to
   * store the contents of the image as an undo delta object, which can then
   * be stored in memory compactly. The caller is responsible for deleting the
   * array created in this call. */
  UndoManagerDelta *CompressImage() const;

protected:

  LabelImageWrapper();
  ~LabelImageWrapper();

  // Undo data manager, stores 'deltas', i.e., differences between states of the segmentation
  // image. These deltas are compressed, allowing us to store a bunch of
  // undo steps with little cost in performance or memory
  UndoManagerType *m_UndoManager;
};

#endif // LABELIMAGEWRAPPER_H
