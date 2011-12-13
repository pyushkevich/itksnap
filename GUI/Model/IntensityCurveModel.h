#ifndef INTENSITYCURVEMODEL_H
#define INTENSITYCURVEMODEL_H

#include <AbstractModel.h>
#include <ImageWrapperBase.h>
#include <UIReporterDelegates.h>

#include "GenericImageData.h"
#include <LayerAssociation.h>

#include "EditableNumericValueModel.h"

class GlobalUIModel;
class IntensityCurveModel;



/**
  A set of properties associated with a specific layer
  */
class IntensityCurveLayerProperties
{
public:
  /** Desired histogram bin size, in pixels */
  irisGetSetMacro(HistogramBinSize, unsigned int)

  irisIsMacro(HistogramLog)
  irisSetMacro(HistogramLog, bool)

  /** How much of the height of the histogram is shown */
  irisGetSetMacro(HistogramCutoff, double)

  /** The control point currently moving */
  irisGetSetMacro(MovingControlPoint, int)

  irisGetSetMacro(ObserverTag, unsigned long)

  IntensityCurveLayerProperties();
  ~IntensityCurveLayerProperties();


protected:

  unsigned int m_HistogramBinSize;
  bool m_HistogramLog;
  double m_HistogramCutoff;

  int m_MovingControlPoint;

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

struct IntensityCurvePropertyAssociationFactory
{
  IntensityCurveLayerProperties *New(GreyImageWrapperBase *layer);
  IntensityCurveModel *m_Model;
};


/**
  The intensity curve model is used to interact with the intensity curve in
  the adjust contrast user interface. The model is associated with a single
  layer, or no layer at all. There is a one-to-one relationship between the
  model and the view, and a one-to-many relationship between the model and
  the image layers. For each layer, the model maintains a properties object
  of type IntensityCurveLayerProperties.
  */
class IntensityCurveModel : public AbstractModel
{
public:

  irisITKObjectMacro(IntensityCurveModel, AbstractModel)


  irisGetMacro(ParentModel, GlobalUIModel *)
  void SetParentModel(GlobalUIModel *parent);

  /** Before using the model, it must be coupled with a size reporter */
  irisGetSetMacro(ViewportReporter, ViewportSizeReporter *)

  /**
    Set the layer with which the model is associated. This can be NULL,
    in which case, the model will be dissasociated from all layers.
    */
  void SetLayer(GreyImageWrapperBase *layer);

  /** Get the layer associated with the model, or NULL if there is none */
  irisGetMacro(Layer, GreyImageWrapperBase *)

  /**
    Get the curve stored in the current layer
    */
  IntensityCurveInterface *GetCurve();

  /**
    Get the histogram of the current layer
    */
  const ScalarImageHistogram *GetHistogram();

  /**
    Get the properties for the current layer
    */
  IntensityCurveLayerProperties &GetProperties();

  /**
    Process curve interaction event
    */
  bool ProcessMousePressEvent(const Vector3d &x);
  bool ProcessMouseDragEvent(const Vector3d &x);
  bool ProcessMouseReleaseEvent(const Vector3d &x);

  /**
    Create a new property object for a new layer
    */
  IntensityCurveLayerProperties *CreateProperty(GreyImageWrapperBase *w);

  /** Moving control point access */
  bool IsMovingControlPointAvailable();

  Vector2d GetMovingControlPointPosition();
  void SetMovingControlPointPosition(Vector2d p);
  NumericValueRange<Vector2d> GetMovingControlPointRange();



  typedef AbstractEditableNumericValueModel<double> NumericValueModel;

  irisGetMacro(MovingControlXModel, NumericValueModel *)
  irisGetMacro(MovingControlYModel, NumericValueModel *)

protected:

  // A layer association
  typedef LayerAssociation<
    IntensityCurveLayerProperties,GreyImageWrapperBase,
    IntensityCurvePropertyAssociationFactory> LayerMapType;

  // A size reporter delegate
  ViewportSizeReporter *m_ViewportReporter;

  // A set of properties associated with each layer
  LayerMapType m_LayerProperties;

  GlobalUIModel *m_ParentModel;
  GreyImageWrapperBase *m_Layer;

  // The child models for control point X and Y coordinates
  SmartPtr<NumericValueModel> m_MovingControlXModel, m_MovingControlYModel;

  // Whether the control point is being dragged
  bool m_FlagDraggedControlPoint;

  IntensityCurveModel();
  virtual ~IntensityCurveModel();


  Vector3d GetEventCurveCoordiantes(const Vector3d &x);
  bool UpdateControlPoint(size_t i, float t, float x);
  int GetControlPointInVicinity(float x, float y, int pixelRadius);

  friend class IntensityCurvePropertyAssociationFactory;
};

#endif // INTENSITYCURVEMODEL_H
