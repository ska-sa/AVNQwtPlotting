//System includes
#include <cmath>
#include <iostream>

//Library includes
#include <QPen>
#include <QThread>
#include <QPrinter>
#include <QPrintDialog>
#include <QDebug>
#include <qwt_scale_engine.h>
#include <qwt_picker.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_plot_renderer.h>

//Local includes
#include "BasicQwtLinePlotWidget.h"
#include "ui_BasicQwtLinePlotWidget.h"
#include "AVNUtilLibs/Timestamp/Timestamp.h"

using namespace std;

cBasicQwtLinePlotWidget::cBasicQwtLinePlotWidget(QWidget *pParent) :
    QWidget(pParent),
    m_pUI(new Ui::cBasicQwtLinePlotWidget),
    m_bIsGridShown(true),
    m_bShowVerticalLines(true),
    m_bIsPaused(false),
    m_bIsAutoscaleEnabled(false),
    m_u32Averaging(1),
    m_bTimestampInTitleEnabled(true),
    m_bDoLogConversion(false),
    m_bDoPowerLogConversion(false),
    m_bRejectData(true)
{
    m_pUI->setupUi(this);

    //Black background canvas and grid lines by default
    //The background colour is not currently changable. A mutator can be added as necessary
    m_pUI->qwtPlot->setCanvasBackground(QBrush(QColor(Qt::black)));
    showPlotGrid(true);

    //Make the axis and title font a little bit smaller
    m_oYFont = m_pUI->qwtPlot->axisTitle(QwtPlot::yLeft).font();
    m_oYFont.setPointSizeF(m_oYFont.pointSizeF() * 0.75);

    m_oXFont = m_pUI->qwtPlot->axisTitle(QwtPlot::xBottom).font();
    m_oXFont.setPointSizeF(m_oXFont.pointSizeF() * 0.75);

    m_oTitleFont = m_pUI->qwtPlot->titleLabel()->font();
    m_oTitleFont.setPointSizeF(m_oTitleFont.pointSizeF() * 0.7);

    //Set up other plot controls
    m_pPlotPicker = new cBasicQwtLinePlotPicker(QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::RectRubberBand, QwtPicker::AlwaysOn, m_pUI->qwtPlot->canvas());
    m_pPlotPicker->setTrackerPen(QPen(Qt::white));

    m_pPlotZoomer = new QwtPlotZoomer(m_pUI->qwtPlot->canvas());
    m_pPlotZoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    m_pPlotZoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
    m_pPlotZoomer->setRubberBandPen(QPen(Qt::white));
    m_pPlotZoomer->setTrackerMode(QwtPicker::AlwaysOff); //Use our own picker.

    m_pPlotPanner = new QwtPlotPanner(m_pUI->qwtPlot->canvas());
    m_pPlotPanner->setAxisEnabled(QwtPlot::yRight, false);
    m_pPlotPanner->setMouseButton(Qt::MidButton);

    m_pPlotMagnifier = new QwtPlotMagnifier(m_pUI->qwtPlot->canvas());
    m_pPlotMagnifier->setAxisEnabled(QwtPlot::yRight, false);
    m_pPlotMagnifier->setMouseButton(Qt::NoButton);
    m_pPlotMagnifier->setWheelFactor(1.1);

    m_pUI->qwtPlot->setAutoReplot(true);

    //Create colours for the plot curves
    m_qveCurveColours.push_back(Qt::red);
    m_qveCurveColours.push_back(Qt::green);
    m_qveCurveColours.push_back(Qt::blue);
    m_qveCurveColours.push_back(Qt::yellow);
    m_qveCurveColours.push_back(Qt::darkRed);
    m_qveCurveColours.push_back(Qt::darkGreen);
    m_qveCurveColours.push_back(Qt::darkBlue);
    m_qveCurveColours.push_back(Qt::darkYellow);

    connectSignalsToSlots();
}

cBasicQwtLinePlotWidget::~cBasicQwtLinePlotWidget()
{
    for(unsigned int uiChannelNo = 0; uiChannelNo < (unsigned)m_qvpPlotCurves.size(); uiChannelNo++)
    {
        delete m_qvpPlotCurves[uiChannelNo];
    }

    delete m_pUI;
}

void cBasicQwtLinePlotWidget::connectSignalsToSlots()
{
    QObject::connect(m_pUI->pushButton_pauseResume, SIGNAL(clicked()), this, SLOT(slotPauseResume()));
    QObject::connect(m_pUI->checkBox_autoscale, SIGNAL(clicked(bool)), this, SLOT(slotEnableAutoscale(bool)));
    QObject::connect(m_pUI->checkBox_showLegend, SIGNAL(clicked(bool)), this, SLOT(slotShowLegend(bool)));
    QObject::connect(m_pUI->pushButton_grabFrame, SIGNAL(clicked()), this, SLOT(slotGrabFrame()) );

    //Connections to update plot data as well as labels and scales are forced to be queued as the actual drawing of the widget needs to be done in the GUI thread
    //This allows an update request to come from an arbirary thread to get executed by the GUI thread
    qRegisterMetaType<QVector<double> >("QVector<double>");
    qRegisterMetaType<int64_t>("int64_t");
    QObject::connect(this, SIGNAL(sigUpdatePlotData(unsigned int,QVector<double>,QVector<double>,int64_t)),
                     this, SLOT(slotUpdatePlotData(unsigned int,QVector<double>,QVector<double>,int64_t)), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(sigUpdateScalesAndLabels()), this, SLOT(slotUpdateScalesAndLabels()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sigUpdateXScaleBase(int)), this, SLOT(slotUpdateXScaleBase(int)), Qt::QueuedConnection);
}

void cBasicQwtLinePlotWidget::insertWidgetIntoControlFrame(QWidget* pNewWidget, uint32_t u32Index, bool bAddSpacerAfter)
{
    QHBoxLayout* pLayout = qobject_cast<QHBoxLayout*>(m_pUI->frame_controls->layout());

    //Add the new widget
    pLayout->insertWidget(u32Index, pNewWidget);

    if(bAddSpacerAfter)
        pLayout->insertSpacerItem(u32Index + 1, new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));
}


void cBasicQwtLinePlotWidget::addData(const QVector<float> &qvfXData, const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us)
{
    //Safety flag which can be set by other threads if data is known not to be interpretable
    if(m_bRejectData)
        return;

    //Update X data
    processXData(qvfXData, i64Timestamp_us);

    //Update Y data
    processYData(qvvfYData, i64Timestamp_us);

    //Check if number of points to plot is base 2:
    double dExponent = log2(m_qvdXDataToPlot.size());
    double dIntegerPart; //Unused

    if(modf(dExponent, &dIntegerPart) == 0.0)
    {
        sigUpdateXScaleBase(2);
        //Plot grid lines along base 2 numbers
    }
    else
    {
        sigUpdateXScaleBase(10);
        //Otherwise plot grid lines along base 10 numbers
    }

    //Do log conversions if required
    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
    {
        m_oMutex.lockForRead(); //Ensure the 2 bool flags don't change during these operations

        if(m_bDoLogConversion)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 10 * log10(m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] + 0.001);
            }
        }

        if(m_bDoPowerLogConversion)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 20 * log10(m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo]  + 0.001);
            }
        }

        m_oMutex.unlock();
    }

    m_oMutex.lockForRead(); //Lock for pause flag

    if(!m_bIsPaused)
    {
        //Check that there are enough curves for this data
        updateCurves();

        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvpPlotCurves.size(); u32ChannelNo++)
        {
            sigUpdatePlotData(u32ChannelNo, m_qvdXDataToPlot, m_qvvdYDataToPlot[u32ChannelNo], i64Timestamp_us);
        }
    }

    m_oMutex.unlock();
}

void cBasicQwtLinePlotWidget::processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us)
{
    //This function populates the m_qvdXDataToPlot vector
    //In this basic implementation the input data is simply copied to the output array
    //In derived versions of the class this function should be overloaded

    Q_UNUSED(i64Timestamp_us);

    //Update number of samples in each channel
    m_qvdXDataToPlot.resize(qvfXData.size());

    //Copy the input data to plot array
    for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)qvfXData.size(); u32SampleNo++)
    {
        m_qvdXDataToPlot[u32SampleNo] = qvfXData[u32SampleNo];
    }

    cout << "cBasicQwtLinePlotWidget::processXData(): m_qvdXDataToPlot is  " << m_qvdXDataToPlot.size() << " samples long." << endl;
}


void cBasicQwtLinePlotWidget::processYData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us)
{
    //This function populates the m_qvvdYDataToPlot vector
    //In this basic implementation the input data is simply copied to the output array
    //In derived versions of the class this function should be overloaded

    Q_UNUSED(i64Timestamp_us);

    //Check that our output array has the right number of channels
    if(m_qvvdYDataToPlot.size() != qvvfYData.size())
    {
        m_qvvdYDataToPlot.resize(qvvfYData.size());
    }

    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)qvvfYData.size(); u32ChannelNo++)
    {
        //Update number of samples in each channel
        m_qvvdYDataToPlot[u32ChannelNo].resize(qvvfYData[u32ChannelNo].size());

        //Copy the input data to plot array
        for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)qvvfYData[u32ChannelNo].size(); u32SampleNo++)
        {
            m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = qvvfYData[u32ChannelNo][u32SampleNo];
        }
    }
}

void cBasicQwtLinePlotWidget::setXLabel(const QString &qstrXLabel)
{
    m_qstrXLabel = qstrXLabel;

    sigUpdateScalesAndLabels();
}

void cBasicQwtLinePlotWidget::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;

    sigUpdateScalesAndLabels();
}

void cBasicQwtLinePlotWidget::setYLabel(const QString &qstrYLabel)
{
    m_qstrYLabel = qstrYLabel;

    sigUpdateScalesAndLabels();
}

void cBasicQwtLinePlotWidget::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;

    sigUpdateScalesAndLabels();
}

void cBasicQwtLinePlotWidget::setTitle(const QString &qstrTitle)
{
    m_qstrTitle = qstrTitle;

    sigUpdateScalesAndLabels();
}

void cBasicQwtLinePlotWidget::setCurveNames(const QVector<QString> &qvqstrCurveNames)
{
    m_qvqstrCurveNames = qvqstrCurveNames;

    sigUpdateScalesAndLabels();
}

void cBasicQwtLinePlotWidget::showPlotGrid(bool bEnable)
{
    m_bIsGridShown = bEnable;
    if(m_bIsGridShown)
    {
        QColor oGridlineColour(Qt::gray);
        oGridlineColour.setAlphaF(0.7);

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
        m_oPlotGrid.setMajPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#else
        m_oPlotGrid.setMajorPen(QPen(Qt::gray, 0.5, Qt::SolidLine ));
#endif

        m_oPlotGrid.attach(m_pUI->qwtPlot);
    }
    else
    {
        m_oPlotGrid.detach();
    }
}

void cBasicQwtLinePlotWidget::showAutoscaleControl(bool bEnable)
{
    m_pUI->checkBox_autoscale->setVisible(bEnable);
}

void cBasicQwtLinePlotWidget::showPauseControl(bool bEnable)
{

    m_pUI->pushButton_pauseResume->setVisible(bEnable);
}

void cBasicQwtLinePlotWidget::showLegendControl(bool bEnable)
{
    m_pUI->checkBox_showLegend->setVisible(bEnable);
}

void cBasicQwtLinePlotWidget::slotPauseResume()
{
    slotPause(!m_bIsPaused);
}

void cBasicQwtLinePlotWidget::slotPause(bool bPause)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bIsPaused = bPause;

    if(m_bIsPaused)
        m_pUI->pushButton_pauseResume->setText(QString("Resume"));
    else
        m_pUI->pushButton_pauseResume->setText(QString("Pause"));
}

void cBasicQwtLinePlotWidget::slotEnableAutoscale(bool bEnable)
{
    //If autoscale is being disabled set the zoom base the current Y zoom
    //This allows the user to strobe the autoscale to get a useful zoom base
    if(m_bIsAutoscaleEnabled && !bEnable)
    {
        QRectF oZoomBase = m_pPlotZoomer->zoomBase();
        QwtInterval oAutoscaledYRange = m_pUI->qwtPlot->axisInterval(QwtPlot::yLeft);

        oZoomBase.setTop(oAutoscaledYRange.maxValue());
        oZoomBase.setBottom(oAutoscaledYRange.maxValue());

        m_pPlotZoomer->setZoomBase(oZoomBase);
    }

    m_bIsAutoscaleEnabled = bEnable;

    m_pUI->qwtPlot->setAxisAutoScale(QwtPlot::yLeft, m_bIsAutoscaleEnabled);

    cout << "cBasicQwtLinePlotWidget::slotEnableAutoscale() Autoscale for plot \"" << m_qstrTitle.toStdString() << "\" set to " << m_bIsAutoscaleEnabled << endl;
}

void cBasicQwtLinePlotWidget::slotShowLegend(bool bEnable)
{
    if(bEnable && m_qvpPlotCurves.size())
    {
        QwtLegend *pLegend = new QwtLegend;
        pLegend->setDefaultItemMode( QwtLegendData::Checkable );
        m_pUI->qwtPlot->insertLegend( pLegend, QwtPlot::RightLegend );

        //Align widgets checked/unchecked with whether the plot item is visible
        QwtPlotItemList plotItems = m_pUI->qwtPlot->itemList();
        for(uint32_t u32ItemNo = 0; u32ItemNo < (uint32_t)plotItems.size(); u32ItemNo++)
        {
            QList<QWidget *> legendWidgets = pLegend->legendWidgets( m_pUI->qwtPlot->itemToInfo(plotItems[u32ItemNo]) );

            if (legendWidgets.size() == 1)
            {
                QwtLegendLabel *pLegendLabel = qobject_cast<QwtLegendLabel *>( legendWidgets[0] );

                if (pLegendLabel)
                    pLegendLabel->setChecked(plotItems[u32ItemNo]->isVisible());
            }
        }

        QObject::connect( pLegend, SIGNAL( checked( const QVariant &, bool, int ) ), SLOT( slotLegendChecked( const QVariant &, bool ) ) );
    }
    else
    {
        m_pUI->qwtPlot->insertLegend( NULL );
    }
}

void cBasicQwtLinePlotWidget::enableTimestampInTitle(bool bEnable)
{
    m_bTimestampInTitleEnabled = bEnable;

    if(!m_bTimestampInTitleEnabled)
        m_pUI->qwtPlot->setTitle(m_qstrTitle);
}

void cBasicQwtLinePlotWidget::enableLogConversion(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bDoLogConversion = bEnable;

    if(m_bDoLogConversion)
        m_bDoPowerLogConversion = false;
}

void cBasicQwtLinePlotWidget::enablePowerLogConversion(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bDoPowerLogConversion = bEnable;

    if(m_bDoPowerLogConversion)
        m_bDoLogConversion = false;
}

void cBasicQwtLinePlotWidget::enableRejectData(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bRejectData = bEnable;
}

void cBasicQwtLinePlotWidget::slotUpdatePlotData(unsigned int uiCurveNo, QVector<double> qvdXData, QVector<double> qvdYData, int64_t i64Timestamp_us)
{
    //This function sends data to the actually plot widget in the GUI thread. This is necessary as draw the curve (i.e. updating the GUI) must be done in the GUI thread.
    //Connections to this slot should be queued if from signals not orginating from the GUI thread.

    //cout << "cBasicQwtLinePlotWidget::slotUpdatePlotData() Thread is: " << QThread::currentThread() << endl;

    if(uiCurveNo >= (unsigned int)m_qvpPlotCurves.size())
    {
        cout << "cBasicQwtLinePlotWidget::slotUpdatePlotData(): Warning: Requested plotting for curve index "
             << uiCurveNo << " which is out of range [0, " << m_qvpPlotCurves.size() - 1 << "]. Ignoring." << endl;

        return;
    }

    m_qvpPlotCurves[uiCurveNo]->setSamples(qvdXData, qvdYData);

    //Update horizontal scale
    QRectF oRect = m_pPlotZoomer->zoomBase();
    if(oRect.left() != qvdXData.first() || oRect.right() != qvdXData.last() )
    {
        oRect.setLeft(qvdXData.first() );
        oRect.setRight(qvdXData.last() );

        m_pPlotZoomer->setZoomBase(oRect);
        m_pPlotZoomer->zoom(oRect);
    }

    //Update timestamp in Title if needed
    if(m_bTimestampInTitleEnabled)
    {
        m_pUI->qwtPlot->setTitle( QString("%1 - %2").arg(m_qstrTitle).arg(AVN::stringFromTimestamp_full(i64Timestamp_us).c_str()) );
    }
}

void cBasicQwtLinePlotWidget::slotUpdateScalesAndLabels()
{
    if(m_qstrXUnit.length())
    {
        QwtText oXLabel(QwtText(QString("%1 [%2]").arg(m_qstrXLabel).arg(m_qstrXUnit)) );
        oXLabel.setFont(m_oXFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, oXLabel);
    }
    else
    {
        QwtText oXLabel( QwtText(QString("%1").arg(m_qstrXLabel)) );
        oXLabel.setFont(m_oXFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, oXLabel);
    }

    if(m_qstrYUnit.length())
    {
        QwtText oYLabel( QString("%1 [%2]").arg(m_qstrYLabel).arg(m_qstrYUnit) );
        oYLabel.setFont(m_oYFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, oYLabel);
    }
    else
    {
        QwtText oYLabel( QString("%1").arg(m_qstrYLabel) );
        oYLabel.setFont(m_oYFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, oYLabel);
    }

    m_pPlotPicker->setXUnit(m_qstrXUnit);
    m_pPlotPicker->setYUnit(m_qstrYUnit);

    QwtText oTitle(m_qstrTitle);
    oTitle.setFont(m_oTitleFont);
    m_pUI->qwtPlot->setTitle(oTitle);

    for(uint32_t u32CurveNo = 0; u32CurveNo < (uint32_t)m_qvpPlotCurves.size() && u32CurveNo < (uint32_t)m_qvqstrCurveNames.size(); u32CurveNo++)
    {
        m_qvpPlotCurves[u32CurveNo]->setTitle(m_qvqstrCurveNames[u32CurveNo]);
    }
}

void cBasicQwtLinePlotWidget::slotGrabFrame()
{
    slotPause(true);

    QwtPlotRenderer oRenderer;
    oRenderer.exportTo(m_pUI->qwtPlot, QString("%1").arg(m_pUI->qwtPlot->title().text()), QSizeF(297.0, 210.0), 300);

    slotPause(false);
}

void cBasicQwtLinePlotWidget::slotUpdateXScaleBase(int iBase)
{
    m_pUI->qwtPlot->axisScaleEngine(QwtPlot::xBottom)->setBase(iBase);
}

void cBasicQwtLinePlotWidget::slotLegendChecked(const QVariant &oItemInfo, bool bChecked)
{
    QwtPlotItem *pPlotItem = m_pUI->qwtPlot->infoToItem(oItemInfo);

    if(pPlotItem)
        showCurve(pPlotItem, bChecked);
}

void  cBasicQwtLinePlotWidget::showCurve(QwtPlotItem *pItem, bool bShow)
{
    pItem->setVisible(bShow);

    m_pUI->qwtPlot->replot();
}

void cBasicQwtLinePlotWidget::updateCurves()
{
    //Make sure that there is a curve for each channel
    if(m_qvpPlotCurves.size() != m_qvvdYDataToPlot.size())
    {
        //If not, delete existing curves
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvpPlotCurves.size(); u32ChannelNo++)
        {
            delete m_qvpPlotCurves[u32ChannelNo];
        }
        m_qvpPlotCurves.clear();

        //Create new curves
        for(unsigned int u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
        {
            if(u32ChannelNo < (uint32_t)m_qvqstrCurveNames.size())
            {
                m_qvpPlotCurves.push_back(new QwtPlotCurve(m_qvqstrCurveNames[u32ChannelNo]));
            }
            else
            {
                m_qvpPlotCurves.push_back(new QwtPlotCurve(QString("Channel %1").arg(u32ChannelNo)));
            }
            m_qvpPlotCurves[u32ChannelNo]->attach(m_pUI->qwtPlot);
            m_qvpPlotCurves[u32ChannelNo]->setPen(m_qveCurveColours[u32ChannelNo], 1.0, Qt::SolidLine);
        }
    }
}

void cBasicQwtLinePlotWidget::slotDrawVerticalLines(QVector<double> qvdXValues)
{
    for(uint32_t u32LineNo = 0; u32LineNo < (uint32_t)m_qvpVerticalLines.size(); u32LineNo++)
    {
        m_qvpVerticalLines[u32LineNo]->detach();
        delete m_qvpVerticalLines[u32LineNo];
    }

    m_qvpVerticalLines.resize(qvdXValues.size());

    for(uint32_t u32LineNo = 0; u32LineNo < (uint32_t)m_qvpVerticalLines.size(); u32LineNo++)
    {
        m_qvpVerticalLines[u32LineNo] = new QwtPlotMarker();
        m_qvpVerticalLines[u32LineNo]->setXValue(qvdXValues[u32LineNo]);
        m_qvpVerticalLines[u32LineNo]->setLinePen(QColor(Qt::cyan));
        m_qvpVerticalLines[u32LineNo]->setLineStyle(QwtPlotMarker::VLine);
        m_qvpVerticalLines[u32LineNo]->setLabelOrientation(Qt::Vertical);
        m_qvpVerticalLines[u32LineNo]->setLabelAlignment(Qt::AlignLeft);
        if(m_bShowVerticalLines)
            m_qvpVerticalLines[u32LineNo]->attach(m_pUI->qwtPlot);
    }
}

void cBasicQwtLinePlotWidget::slotDrawVerticalLines(QVector<double> qvdXValues, QVector<QString> qvqstrLabels)
{
    slotDrawVerticalLines(qvdXValues);

    for(uint32_t u32LineNo = 0; u32LineNo < (uint32_t)qvqstrLabels.size(); u32LineNo++)
    {
        if((uint32_t)m_qvpVerticalLines.size() > u32LineNo)
        {
            m_qvpVerticalLines[u32LineNo]->setTitle(qvqstrLabels[u32LineNo]);
            m_qvpVerticalLines[u32LineNo]->setLabel(qvqstrLabels[u32LineNo]);
        }
    }
}

void cBasicQwtLinePlotWidget::slotShowVerticalLines(bool bShow)
{
    m_bShowVerticalLines = bShow;

    for(uint32_t u32LineNo = 0; u32LineNo < (uint32_t)m_qvpVerticalLines.size(); u32LineNo++)
    {
        m_qvpVerticalLines[u32LineNo]->detach();
    }

    if(m_bShowVerticalLines)
    {
        for(uint32_t u32LineNo = 0; u32LineNo < (uint32_t)m_qvpVerticalLines.size(); u32LineNo++)
        {
            m_qvpVerticalLines[u32LineNo]->attach(m_pUI->qwtPlot);
        }
    }
}
