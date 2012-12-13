#ifndef AXES_WIDGET_H
#define AXES_WIDGET_H

#include "vtkObject.h"
#include "vtkAxesActor.h"
#include "vtkSmartPointer.h"

class AxesWidget : public vtkObject {
	AxesWidget();
	//virtual ~AxesWidget();
	//virtual void Delete();
public:

	//static AxesWidget *New();

	virtual void VisibilityOn();
	virtual void VisibilityOff();
	virtual int GetVisibility();
	virtual void SetVisibility(int anVisible);

	static AxesWidget * New();
	

	void SetLengths(double adbLength);

	vtkSmartPointer < vtkAxesActor > GetAxesActor();

	void SetLabels(const char * apchX, const char * apchY, const char * apchZ);
protected:
	vtkSmartPointer < vtkAxesActor > m_pvtkAxesActor;
private:
};

#endif //AXES_WIDGET_H