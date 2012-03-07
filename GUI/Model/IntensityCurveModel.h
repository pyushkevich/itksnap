#ifndef INTENSITYCURVEMODEL_H
#define INTENSITYCURVEMODEL_H

#include <AbstractLayerAssociatedModel.h>
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

typedef AbstractLayerAssociatedModel<
    IntensityCurveLayerProperties,
    GreyImageWrapperBase> IntensityCurveModelBase;

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
  void RegisterWithLayer(GreyImageWrapperBase *layer);
  void UnRegisterFromLayer(GreyImageWrapperBase *layer);

  /**
    Get the curve stored in the current layer
    */
  IntensityCurveInterface *GetCurve();

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

  /**
    Create a new property object for a new layer
    */
  IntensityCurveLayerProperties *CreateProperty(GreyImageWrapperBase *w);



  /** Update the model in response to upstream events */
  virtual void OnUpdate();

  typedef AbstractEditableNumericValueModel<double> RealValueModel;
  typedef AbstractEditableNumericValueModel<int> IntegerValueModel;
  typedef AbstractEditableNumericValueModel<bool> BooleanValueModel;

  irisGetMacro(MovingControlXModel, RealValueModel *)
  irisGetMacro(MovingControlYModel, RealValueModel *)
  irisGetMacro(LevelModel, RealValueModel *)
  irisGetMacro(WindowModel, RealValueModel *)

  irisGetMacro(MovingControlIdModel, IntegerValueModel *)

  irisGetMacro(HistogramBinSizeModel, IntegerValueModel *)

  irisGetMacro(HistogramCutoffModel, RealValueModel *)

  irisGetMacro(HistogramScaleModel, BooleanValueModel *)


  void OnAutoFitWindow();
protected:

  IntensityCurveModel();
  virtual ~IntensityCurveModel();

  // A size reporter delegate
  ViewportSizeReporter *m_ViewportReporter;

  // Whether the control point is being dragged
  bool m_FlagDraggedControlPoint;

  Vector3d GetEventCurveCoordiantes(const Vector3d &x);
  bool UpdateControlPoint(size_t i, float t, float x);
  int GetControlPointInVicinity(float x, float y, int pixelRadius);

  // Model for the control point index
  SmartPtr<IntegerValueModel> m_MovingControlIdModel;

  // Moving control point Id access methods
  bool GetMovingControlPointIdValueAndRange(int &value,
                                            NumericValueRange<int> *range);
  void SetMovingControlPointId(int value);

  // The child models for control point X and Y coordinates
  SmartPtr<RealValueModel> m_MovingControlXModel, m_MovingControlYModel;

  // Moving control point position access methods
  bool GetMovingControlPointPositionAndRange(Vector2d &lw,
                                             NumericValueRange<Vector2d> *range);
  void SetMovingControlPointPosition(Vector2d p);

  // Child models for window and level
  SmartPtr<RealValueModel> m_LevelModel, m_WindowModel;

  // Window and level access methods
  bool GetLevelAndWindowValueAndRange(Vector2d &lw,
                                      NumericValueRange<Vector2d> *range);
  void SetLevelAndWindow(Vector2d p);

  // Child model for histogram bin size
  SmartPtr<IntegerValueModel> m_HistogramBinSizeModel;

  // Histogram bin size access methods
  bool GetHistogramBinSizeValueAndRange(int &value,
                                        NumericValueRange<int> *range);
  void SetHistogramBinSize(int value);

  // Child model for histogram cutoff
  SmartPtr<RealValueModel> m_HistogramCutoffModel;

  // Histogram bin size access methods
  bool GetHistogramCutoffValueAndRange(double &value,
                                       NumericValueRange<double> *range);
  void SetHistogramCutoff(double value);

  // Child model for histogram scale
  SmartPtr<BooleanValueModel> m_HistogramScaleModel;

  // Histogram bin size access methods
  bool GetHistogramScaleValueAndRange(bool &value,
                                      NumericValueRange<bool> *range);
  void SetHistogramScale(bool value);

};

#endif // INTENSITYCURVEMODEL_H
