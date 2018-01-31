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
#include "ImageIODelegates.h"
#include "ImageIOWizard.h"
#include "ImageIOWizardModel.h"

#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkWatershedImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkCommand.h>


void FilterEventHandlerITK(itk::Object *caller, const itk::EventObject &event, void*){

    const itk::ProcessObject* filter = static_cast<const itk::ProcessObject*>(caller);

    if(itk::ProgressEvent().CheckEvent(&event))
	fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetNameOfClass(), 100.0 * filter->GetProgress());//stderr is flushed directly
    else if(itk::EndEvent().CheckEvent(&event))
	std::cerr << std::endl << std::flush;
    }


// TODO: move this into a separate file!!!!
class WatershedPipeline{
public:
    typedef itk::Image<GreyType, 3> GreyImageType;
    typedef itk::Image<JSRType, 3> JsrcImageType;
    typedef itk::Image<WSRType, 3> WsrcImageType;
    typedef itk::Image<GWSType, 3> WatershedImageType;

    WatershedPipeline(){

	itk::CStyleCommand::Pointer eventCallbackITK;
	eventCallbackITK = itk::CStyleCommand::New();
	eventCallbackITK->SetCallback(FilterEventHandlerITK);

	adf = ADFType::New();
	adf->AddObserver(itk::ProgressEvent(), eventCallbackITK);
	adf->AddObserver(itk::EndEvent(), eventCallbackITK);
	gmf = GMFType::New();
	gmf->AddObserver(itk::ProgressEvent(), eventCallbackITK);
	gmf->AddObserver(itk::EndEvent(), eventCallbackITK);
	gmf->SetInput(adf->GetOutput());
	wf = WFType::New();
	wf->AddObserver(itk::ProgressEvent(), eventCallbackITK);
	wf->AddObserver(itk::EndEvent(), eventCallbackITK);
	cif = CIFType::New();
	cif->SetInput(wf->GetOutput());
	}

    WsrcImageType* ComputeWSImage(
	GreyImageType *grey,
	double cParam, size_t sIter,
	bool direct){

	//// Initialize the watershed pipeline
	adf->SetInput(grey);
	adf->SetNumberOfIterations(sIter);
	adf->SetConductanceParameter(cParam);

        WsrcImageType* wsImage;
        
	if(direct){
            adf->UpdateLargestPossibleRegion();
	    wsImage= adf->GetOutput();
            }
	else{
            gmf->UpdateLargestPossibleRegion();
            wsImage= gmf->GetOutput();
            }
        
        wf->SetInput(wsImage);//causes recomp even when no params were chanaged -> todo: optimize by setting input only if it changed
	return(wsImage);
	}

    JsrcImageType* PrecomputeWatersheds(double iThr, double iLevel){

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
    typedef itk::GradientAnisotropicDiffusionImageFilter<GreyImageType,WsrcImageType> ADFType;
    typedef itk::GradientMagnitudeImageFilter<WsrcImageType, WsrcImageType> GMFType;
    typedef itk::WatershedImageFilter<WsrcImageType> WFType;
    typedef itk::CastImageFilter<WatershedImageType, JsrcImageType> CIFType;

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
    // Get the global objects
    IRISApplication *driver = m_ParentModel->GetDriver();
    GlobalState *gs = driver->GetGlobalState();

    try
	{
	// Handle cursor
	QtCursorOverride curse(Qt::WaitCursor);
                
	driver->GetJOINImageData()->SetWsrc(
	    m_Watershed->ComputeWSImage(
		driver->GetCurrentImageData()->GetMain()->GetDefaultScalarRepresentation()->GetCommonFormatImage(),
		ui->inConductance->value()/100.0, ui->inSmoothingIter->value(),
		ui->chkGlobalWSDirect->isChecked()
		)
	    );
        // set tiled layout to ease understanding the interaction mode
	driver->GetJOINImageData()->SetWsrcSticky(false);
	m_ParentModel->SetWsrcVisibility(true);
	//m_ParentModel->SetJsrcVisibility(false);//let user decide
	//m_ParentModel->SetJdstVisibility(false);//let user decide
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_STACKED);//only when coming from stacked view will the tiled view get reorganized
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_TILED);

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

	driver->GetJOINImageData()->SetJsrc(
	    m_Watershed->PrecomputeWatersheds(ui->inWSMin->value()/100.0, ui->inWSMax->value()/100.0)
	    );
        // set tiled layout to ease understanding the interaction mode
	driver->GetJOINImageData()->SetJdstSticky(false);
	driver->GetJOINImageData()->SetWsrcSticky(true);
	m_ParentModel->SetWsrcVisibility(false);
	m_ParentModel->SetJsrcVisibility(true);//make sure Jsrc is visible
	m_ParentModel->SetJdstVisibility(true);//make sure Jdst is visible
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_STACKED);//only when coming from stacked view will the tiled view get reorganized
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_TILED);

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
    m_ParentModel->SetSegmentationVisibility(true);
    }

void GlobalWSWizardPanel::on_btnClearSeg_clicked(){
    IRISApplication *driver = m_ParentModel->GetDriver();
    driver->ClearJdst();

    driver->InvokeEvent(SegmentationChangeEvent());
    m_ParentModel->SetSegmentationVisibility(true);
    }

void GlobalWSWizardPanel::on_btnLoadFromFile_clicked(){///better make it a choose overlay button as ROI cropping has already happend
    // not working yet
    // Create a model for IO
    SmartPtr<LoadOverlayImageDelegate> delegate = LoadOverlayImageDelegate::New();
    delegate->Initialize(m_ParentModel->GetDriver());
    SmartPtr<ImageIOWizardModel> model = ImageIOWizardModel::New();
    model->InitializeForLoad(m_ParentModel, delegate,
	"GWSImage", "GWS Source Image");

    // Execute the IO wizard
    ImageIOWizard wiz(this);
    wiz.SetModel(model);
    wiz.exec();
    }

void GlobalWSWizardPanel::on_btnWSRangeBack_clicked(){
    try
	{
	// Handle cursor
	QtCursorOverride curse(Qt::WaitCursor);

	// Move to the range page
	ui->stack->setCurrentWidget(ui->pgPreproc);

        // reset tiled layout
	m_ParentModel->GetDriver()->GetJOINImageData()->SetWsrcSticky(true);
	m_ParentModel->SetWsrcVisibility(false);
	//m_ParentModel->SetJsrcVisibility(false);//let user decide
	//m_ParentModel->SetJdstVisibility(false);//let user decide
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_STACKED);
	}
    catch(IRISException &exc)
	{
	QMessageBox::warning(this, "ITK-SNAP", exc.what(), QMessageBox::Ok);
	}
    }

void GlobalWSWizardPanel::on_btnJoinBack_clicked(){
    try
	{
	// Handle cursor
	QtCursorOverride curse(Qt::WaitCursor);

	// Move to the range page
	ui->stack->setCurrentWidget(ui->pgWSRange);

	// reset tiled layout
	m_ParentModel->GetDriver()->GetJOINImageData()->SetJdstSticky(true);
	m_ParentModel->GetDriver()->GetJOINImageData()->SetWsrcSticky(false);
	m_ParentModel->SetWsrcVisibility(true);
	//m_ParentModel->SetJsrcVisibility(false);//let user decide
	//m_ParentModel->SetJdstVisibility(false);//let user decide
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_STACKED);//only when coming from stacked view will the tiled view get reorganized
	m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel()->SetValue(LAYOUT_TILED);
	}
    catch(IRISException &exc)
	{
	QMessageBox::warning(this, "ITK-SNAP", exc.what(), QMessageBox::Ok);
	}    
    }