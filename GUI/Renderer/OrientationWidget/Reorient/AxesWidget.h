#ifndef AXES_WIDGET_H
#define AXES_WIDGET_H

#include "vtkObject.h"
#include "vtkAxesActor.h"
#include "vtkSmartPointer.h"

class AxesWidget : public vtkObject {
	AxesWidget();

public:

    static const double m_arrdbColRed[3];
    static const double m_arrdbColGreen[3];
    static const double m_arrdbColBlue[3];

	virtual void VisibilityOn();
	virtual void VisibilityOff();
	virtual int GetVisibility();
    virtual void SetVisibility(int anVisibility);

	static AxesWidget * New();
	

	void SetLengths(double adbLength);

	vtkSmartPointer < vtkAxesActor > GetAxesActor();

	void SetLabels(const char * apchX, const char * apchY, const char * apchZ);
    void SetColors(const double aarrdbX[3], const double aarrdbY[3], const double aarrdbZ[3]);
protected:
	vtkSmartPointer < vtkAxesActor > m_pvtkAxesActor;
private:
};

#endif //AXES_WIDGET_H
