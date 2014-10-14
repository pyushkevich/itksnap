/*
part of Click'n'Join mode, which was contributed by Roman Grothausmann
*/

#include "GlobalWSWizardPanel.h"
#include "ui_GlobalWSWizardPanel.h"
#include "GlobalUIModel.h"
#include "GlobalWSWizardModel.h"
#include "QtCursorOverride.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "IRISException.h"
#include <QMessageBox>
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "JOINImageData.h"
#include "DisplayLayoutModel.h" //access tiled/stacked view mode

#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkWatershedImageFilter.h>
#include <itkCastImageFilter.h>

// TODO: move this into a separate file!!!!
class WatershedPipeline{
public:
    typedef itk::Image<GreyType, 3> GreyImageType;
    typedef itk::Image<LabelType, 3> LabelImageType;
    typedef itk::Image<float, 3> FloatImageType;
    typedef itk::Image<GWSType, 3> WatershedImageType;

    WatershedPipeline(){
	adf = ADFType::New();
	gmf = GMFType::New();
	gmf->SetInput(adf->GetOutput());
	wf = WFType::New();
	cif = CIFType::New();
	cif->SetInput(wf->GetOutput());
	}

    LabelImageType* PrecomputeWatersheds(
	GreyImageType *grey,
	double cParam, size_t sIter,
	double iThr, double iLevel, 
	bool direct){

	//// Initialize the watershed pipeline
	adf->SetInput(grey);
	adf->SetNumberOfIterations(sIter);
	adf->SetConductanceParameter(cParam);

	if(direct)
	    wf->SetInput(adf->GetOutput());
	else
	    wf->SetInput(gmf->GetOutput());

	wf->SetThreshold(iThr);
	wf->SetLevel(iLevel);
 	cif->UpdateLargestPossibleRegion(); //needed because RequestedRegion can be bigger than BufferedRegion in consecutive runs of GWS (http://itk.org/ITKExamples/src/Core/Common/ReRunPipelineWithChangingLargestPossibleRegion/Documentation.html)

	return(cif->GetOutput());
	}

    void RecomputeWatersheds(double level){
	// Reupdate the filter with new level
	wf->SetLevel(level);
	cif->Update();
	}

private:
    typedef itk::GradientAnisotropicDiffusionImageFilter<GreyImageType,FloatImageType> ADFType;
    typedef itk::GradientMagnitudeImageFilter<FloatImageType, FloatImageType> GMFType;
    typedef itk::WatershedImageFilter<FloatImageType> WFType;
    typedef itk::CastImageFilter<WatershedImageType, LabelImageType> CIFType;

    ADFType::Pointer adf;
    GMFType::Pointer gmf;
    WFType::Pointer wf;
    CIFType::Pointer cif;

    };



GlobalWSWizardPanel::GlobalWSWizardPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GlobalWSWizardPanel){
    ui->setupUi(this);

    connect(ui->inWSLevel, SIGNAL(valueChanged(double)), this, SLOT(on_inWSLevel_valueChanged(double)));
    m_old_value= 0;

    m_Watershed = new WatershedPipeline();

    this->addAction(ui->actionIncreaseWSLevel);
    this->addAction(ui->actionDecreaseWSLevel);
    }

GlobalWSWizardPanel::~GlobalWSWizardPanel(){
    delete ui;
    }

void GlobalWSWizardPanel::SetModel(GlobalUIModel *model){
    // Store and pass on the models
    m_ParentModel = model;
    m_Model = model->GetGlobalWSWizardModel();

    activateOnFlag(ui->btnCopySeg, m_ParentModel, UIF_JOIN_MODE);
    activateOnFlag(ui->btnClearSeg, m_ParentModel, UIF_JOIN_MODE);
    }

void GlobalWSWizardPanel::Initialize(){
    // Initialize the model
    m_Model->OnGlobalWSModeEnter();

    // set tiled layout to ease understanding the interaction mode
    m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_TILED);

    // Go to the right page
    ui->stack->setCurrentWidget(ui->pgPreproc);
    }

void GlobalWSWizardPanel::on_stack_currentChanged(int page){
    // The stack at the top follows the stack at the bottom
    ui->stackStepInfo->setCurrentIndex(page);
    }

void GlobalWSWizardPanel::on_btnCancel_clicked(){
    // Tell the model to return to initialization state
    m_Model->OnCancelSegmentation();

    // Tell parent to hide this window
    emit wizardFinished();
    }

void GlobalWSWizardPanel::on_btnNextPreproc_clicked(){
    // Call the initialization code
    try
	{
	// Handle cursor
	QtCursorOverride curse(Qt::WaitCursor);

	// Move to the range page
	ui->stack->setCurrentWidget(ui->pgWSRange);
	}
    catch(IRISException &exc)
	{
	QMessageBox::warning(this, "ITK-SNAP", exc.what(), QMessageBox::Ok);
	}
    }

void GlobalWSWizardPanel::on_btnWSRangeNext_clicked(){

    // Get the global objects
    IRISApplication *driver = m_ParentModel->GetDriver();
    GlobalState *gs = driver->GetGlobalState();

    try
	{
	// Handle cursor
	QtCursorOverride curse(Qt::WaitCursor);

	driver->GetJOINImageData()->GetJsrc()->SetImage(
	    m_Watershed->PrecomputeWatersheds(
		driver->GetCurrentImageData()->GetMain()->GetDefaultScalarRepresentation()->GetCommonFormatImage(),
		ui->inConductance->value()/100.0, ui->inSmoothingIter->value(),
		ui->inWSMin->value()/100.0, ui->inWSMax->value()/100.0,
		ui->chkGlobalWSDirect->isChecked()
		)
	    );

	std::cerr << "Changing WS to level: " << ui->inWSLevel->value()/100.0 << std::flush;
	m_Watershed->RecomputeWatersheds(ui->inWSLevel->value()/100.0);
	std::cerr << " Recomputed WS level." << std::flush << std::endl;
	driver->InvokeEvent(SegmentationChangeEvent());

	ui->stack->setCurrentWidget(ui->pgDynWSLevel);
	}
    catch(IRISException &exc)
	{
	QMessageBox::warning(this, "ITK-SNAP", exc.what(), QMessageBox::Ok);
	}

    }

void GlobalWSWizardPanel::on_inWSLevel_valueChanged(double value){

    // Get the global objects
    IRISApplication *driver = m_ParentModel->GetDriver();
    GlobalState *gs = driver->GetGlobalState();

    GenericImageData *gid = driver->GetCurrentImageData();

    if(value != m_old_value){
	try
	    {
	    std::cerr << "Changing WS to level: " << value/100.0 << std::flush;
	    m_Watershed->RecomputeWatersheds(value/100.0);
	    std::cerr << " Recomputed WS level." << std::flush << std::endl;
	    driver->InvokeEvent(SegmentationChangeEvent());
	    }
	catch(IRISException &exc)
	    {
	    QMessageBox::warning(this, "ITK-SNAP", exc.what(), QMessageBox::Ok);
	    }
	m_old_value= value;
	}
    
    }

void GlobalWSWizardPanel::on_btnGWSfinish_clicked(){
    // Tell the model to return to initialization state
    m_Model->OnFinishGWS();

    // Tell parent to hide this window
    emit wizardFinished();
    }

void GlobalWSWizardPanel::on_actionIncreaseWSLevel_triggered(){
    double value= ui->inWSLevel->value() + 0.1;
    ui->inWSLevel->setValue(value);
    emit ui->inWSLevel->spinnerValueChanged(value); //def in GUI/Qt/Components/QDoubleSliderWithEditor.h
    }

void GlobalWSWizardPanel::on_actionDecreaseWSLevel_triggered(){
    double value= ui->inWSLevel->value() - 0.1;
    ui->inWSLevel->setValue(value);
    emit ui->inWSLevel->spinnerValueChanged(value);
    }

void GlobalWSWizardPanel::on_btnCopySeg_clicked(){
    IRISApplication *driver = m_ParentModel->GetDriver();
    driver->CopySegementationToJdst(
	driver->GetGlobalState()->GetSegmentationROISettings(),
	m_ParentModel->GetProgressCommand());

    driver->InvokeEvent(SegmentationChangeEvent());
    }

void GlobalWSWizardPanel::on_btnClearSeg_clicked(){
    IRISApplication *driver = m_ParentModel->GetDriver();
    driver->ClearJdst();

    driver->InvokeEvent(SegmentationChangeEvent());
    }
