#ifndef INTENSITYCURVEMODEL_H
#define INTENSITYCURVEMODEL_H

#include <AbstractLayerAssociatedModel.h>
#include <ImageWrapperBase.h>
#include <UIReporterDelegates.h>

#include "GenericImageData.h"
#include <LayerAssociation.h>

#include "PropertyModel.h"

class GlobalUIModel;
class IntensityCurveModel;
class AbstractContinuousImageDisplayMappingPolicy;


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

  /** Is this the first time we are registered with the layer? */
  irisIsMacro(FirstTime)
  irisSetMacro(FirstTime, bool)

  irisGetSetMacro(ObserverTag, unsigned long)

  IntensityCurveLayerProperties();
  virtual ~IntensityCurveLayerProperties();


protected:

  unsigned int m_HistogramBinSize;
  bool m_HistogramLog;
  double m_HistogramCutoff;

  int m_MovingControlPoint;

  // Whether or not we have been registered with this layer before.
  // By default, this is set to true. This flag allows us to perform
  // some one-off initialization stuff (like set the histogram cutoff)
  // in RegisterWithLayer
  bool m_FirstTime;

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

typedef AbstractLayerAssociatedModel<
    IntensityCurveLayerProperties,
    ImageWrapperBase> IntensityCurveModelBase;

/**
  The intensity curve model is used to interact with the intensity curve in
  the adjust contrast user interface. The model is associated with a single
  layer, or no layer at all. There is a one-to-one relationship between the
  model and the view, and a one-to-many relationship between the model and
  the image layers. For each layer, the model maintains a properties object
  of type IntensityCurveLayerProperties.

  To change the layer with which the model is associated, call SetLayer. The
  model will fire an event, to which the UI should listen, refreshing the UI
  in response.
  */
class IntensityCurveModel : public IntensityCurveModelBase
{
public:

  irisITKObjectMacro(IntensityCurveModel, IntensityCurveModelBase)


  /** Before using the model, it must be coupled with a size reporter */
  irisGetSetMacro(ViewportReporter, ViewportSizeReporter *)

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(ImageWrapperBase *layer);
  void UnRegisterFromLayer(ImageWrapperBase *layer, bool being_deleted);


  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_LAYER_ACTIVE,
    UIF_CONTROL_SELECTED
    };

  /**
    Get the curve stored in the current layer
    */
  IntensityCurveInterface *GetCurve();

  /** Get the input intensity range for the curve (native) */
  Vector2d GetNativeImageRangeForCurve();

  /**
   * Get the current min and max of the curve in native image units
   */
  Vector2d GetCurveRange();

  /**
   * Get the visibile intensity range (in native image intensity units).
   * The lower bound of this range is the minimum of the first control point's
   * intensity and the minimum of the image intensity. The upper bound is the
   * maximum of the last control point's intensity and the maximum image value.
   */
  Vector2d GetVisibleImageRange();

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

  /**
    Get the histogram of the current layer
    */
  const ScalarImageHistogram *GetHistogram();

  /**
    Process curve interaction event
    */
  bool ProcessMousePressEvent(const Vector3d &x);
  bool ProcessMouseDragEvent(const Vector3d &x);
  bool ProcessMouseReleaseEvent(const Vector3d &x);

  /** Reduce number of control points */
  void OnControlPointNumberDecreaseAction();

  /** Increase number of control points */
  void OnControlPointNumberIncreaseAction();

  /** Reset the curve */
  void OnResetCurveAction();

  /** Try changing the control point to new values */
  bool UpdateControlPoint(size_t i, float t, float x);


  /** Update the model in response to upstream events */
  virtual void OnUpdate();

  // Access the models
  irisGetMacro(MovingControlXYModel, AbstractRangedDoubleVec2Property *)
  irisGetMacro(MovingControlIdModel, AbstractRangedIntProperty *)
  irisGetMacro(HistogramBinSizeModel, AbstractRangedIntProperty *)
  irisGetMacro(HistogramCutoffModel, AbstractRangedDoubleProperty *)
  irisGetMacro(HistogramScaleModel, AbstractSimpleBooleanProperty *)

  // This enum lists the types of global intensity range properties for which
  // separate models are defined
  enum IntensityRangePropertyType { MINIMUM = 0, MAXIMUM, LEVEL, WINDOW };

  // Access the models for the intensity min, max, level and window. These
  // models are specified by an index
  AbstractRangedDoubleProperty *GetIntensityRangeModel(
      IntensityRangePropertyType index) const;

  void OnAutoFitWindow();
protected:

  IntensityCurveModel();
  virtual ~IntensityCurveModel();

  AbstractContinuousImageDisplayMappingPolicy *GetDisplayPolicy();

  // A size reporter delegate
  ViewportSizeReporter *m_ViewportReporter;

  // Whether the control point is being dragged
  bool m_FlagDraggedControlPoint;


  Vector3d GetEventCurveCoordiantes(const Vector3d &x);
  int GetControlPointInVicinity(float x, float y, int pixelRadius);

  // Model for the control point index
  SmartPtr<AbstractRangedIntProperty> m_MovingControlIdModel;

  // Moving control point Id access methods
  bool GetMovingControlPointIdValueAndRange(int &value,
                                            NumericValueRange<int> *range);
  void SetMovingControlPointId(int value);

  // The child models for control point X and Y coordinates
  SmartPtr<AbstractRangedDoubleVec2Property> m_MovingControlXYModel;

  // Moving control point position access methods
  bool GetMovingControlPointPositionAndRange(Vector2d &lw,
                                             NumericValueRange<Vector2d> *range);
  void SetMovingControlPointPosition(Vector2d p);

  // Child models for min, max, window and level
  SmartPtr<AbstractRangedDoubleProperty> m_IntensityRangeModel[4];

  // Window and level access methods
  bool GetIntensityRangeIndexedValueAndRange(int index,
                                      double &value,
                                      NumericValueRange<double> *range);
  void SetIntensityRangeIndexedValue(int index, double value);

  // Child model for histogram bin size
  SmartPtr<AbstractRangedIntProperty> m_HistogramBinSizeModel;

  // Histogram bin size access methods
  bool GetHistogramBinSizeValueAndRange(int &value,
                                        NumericValueRange<int> *range);
  void SetHistogramBinSize(int value);

  // Child model for histogram cutoff
  SmartPtr<AbstractRangedDoubleProperty> m_HistogramCutoffModel;

  // Histogram bin size access methods
  bool GetHistogramCutoffValueAndRange(double &value,
                                       NumericValueRange<double> *range);
  void SetHistogramCutoff(double value);

  // Child model for histogram scale
  SmartPtr<AbstractSimpleBooleanProperty> m_HistogramScaleModel;

  // Histogram bin size access methods
  bool GetHistogramScale(bool &value);
  void SetHistogramScale(bool value);

};

#endif // INTENSITYCURVEMODEL_H
