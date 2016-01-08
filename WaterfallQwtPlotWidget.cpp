//System includes
#include <iostream>
#include <cmath>
#include <climits>

//Library includes
#include <qwt_text_label.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>
#include <qwt_interval.h>

//Local includes
#include "WaterfallQwtPlotWidget.h"
#include "ui_QwtPlotWidgetBase.h"

using namespace std;

cWaterfallQwtPlotWidget::cWaterfallQwtPlotWidget(uint32_t u32ChannelNo, const QString &qstrChannelName, QWidget *pParent) :
    cQwtPlotWidgetBase(pParent),
    m_u32AverageCount(0),
    m_pTimeScaleDraw(new cWallTimeQwtScaleDraw),
    m_u32ChannelNo(u32ChannelNo),
    m_qstrChannelName(qstrChannelName),
    m_bAutoscaleValid(false)
{
    //Additional controls to the waterfall widget
    m_pIntensityFloorLabel = new QLabel(QString("Intensity floor"), this);
    m_pIntensityFloorSpinBox = new QDoubleSpinBox(this);
    m_pIntensityFloorSpinBox->setDecimals(1);
    m_pIntensityFloorSpinBox->setMaximum(1e10);
    m_pIntensityFloorSpinBox->setMinimum(-1e10);

    m_pIntensityCeilingLabel = new QLabel(QString("Intensity ceiling"), this);
    m_pIntensityCeilingSpinBox = new QDoubleSpinBox(this);
    m_pIntensityCeilingSpinBox->setDecimals(1);
    m_pIntensityCeilingSpinBox->setMaximum(1e10);
    m_pIntensityCeilingSpinBox->setMinimum(-1e10);

    m_pTimeSpanLabel = new QLabel(QString("Time span"), this);
    m_pTimeSpanSpinBox_s = new QSpinBox(this);
    m_pTimeSpanSpinBox_s->setMinimum(1);
    m_pTimeSpanSpinBox_s->setMaximum(INT32_MAX);
    m_pTimeSpanSpinBox_s->setValue(120); //Default span 2 minutes
    m_pTimeSpanSpinBox_s->setSuffix(QString(" s"));

    insertWidgetIntoControlFrame(m_pIntensityFloorLabel, 3);
    insertWidgetIntoControlFrame(m_pIntensityFloorSpinBox, 4, true);
    insertWidgetIntoControlFrame(m_pIntensityCeilingLabel, 6);
    insertWidgetIntoControlFrame(m_pIntensityCeilingSpinBox, 7, true);
    insertWidgetIntoControlFrame(m_pTimeSpanLabel, 9);
    insertWidgetIntoControlFrame(m_pTimeSpanSpinBox_s, 10, true);

    //Constructs for waterfall plots
    m_pPlotSpectrogram = new QwtPlotSpectrogram;
    m_pSpectrogramData = new cWaterfallPlotSpectromgramData;

    //Automatic assign plot rendering threads based on available hardware
    m_pPlotSpectrogram->setRenderThreadCount(0);

    //Setup the colorMap for the spectrogram
    QwtLinearColorMap *pColourMap = new QwtLinearColorMap(Qt::darkCyan, Qt::red);
    pColourMap->addColorStop(0.25, Qt::cyan);
    pColourMap->addColorStop(0.5, Qt::green);
    pColourMap->addColorStop(0.75, Qt::yellow);
    m_pPlotSpectrogram->setColorMap(pColourMap);

    //Setup the right axis (colour bar)
    m_pColourMap = new QwtLinearColorMap(Qt::darkCyan, Qt::red);
    m_pColourMap->addColorStop(0.25, Qt::cyan);
    m_pColourMap->addColorStop(0.5, Qt::green);
    m_pColourMap->addColorStop(0.75, Qt::yellow);

    m_pUI->qwtPlot->enableAxis(QwtPlot::yRight);
    m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->setColorBarEnabled(true);
    m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->setColorMap(QwtInterval (50, 80), m_pColourMap);
    m_pUI->qwtPlot->setAxisScale(QwtPlot::yRight, 50, 80);
    m_pSpectrogramData->setInterval( Qt::ZAxis, QwtInterval(50.0, 80.0 ) );


    m_oZFont = m_pUI->qwtPlot->axisTitle(QwtPlot::yRight).font();
    m_oZFont.setPointSizeF(m_oZFont.pointSizeF() * 0.65);

    m_pUI->qwtPlot->setAutoReplot(true);
    m_pPlotSpectrogram->attach(m_pUI->qwtPlot);
    m_pPlotSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);

    //The last 2 arguments here back populate timestamps so that the plots starts up with logical values on the timescale
    m_pSpectrogramData->setDimensions(0, 200, AVN::getTimeNow_us(), m_pTimeSpanSpinBox_s->value() * 1000000);

    //Offset width of colour bar in right hand plot spacing
    m_pUI->qwtPlot->axisScaleDraw(QwtPlot::yRight)->setMinimumExtent(m_pUI->qwtPlot->axisScaleDraw(QwtPlot::yRight)->minimumExtent()
                                                                     - m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->colorBarWidth());


    //Install custom time scale drawer for the Y axis
    m_pUI->qwtPlot->setAxisScaleDraw(QwtPlot::yLeft, m_pTimeScaleDraw);

    QObject::connect(this, SIGNAL(sigUpdateData()), this, SLOT(slotUpdateData()), Qt::QueuedConnection);
    QObject::connect(m_pIntensityFloorSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotIntensityFloorChanged(double)) );
    QObject::connect(m_pIntensityCeilingSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotIntensityCeilingChanged(double)) );

    strobeAutoscale();
}

cWaterfallQwtPlotWidget::~cWaterfallQwtPlotWidget()
{
}

void cWaterfallQwtPlotWidget::setZLabel(const QString &qstrZLabel)
{
    m_qstrZLabel = qstrZLabel;

    sigUpdateScalesAndLabels();
}

void cWaterfallQwtPlotWidget::setZUnit(const QString &qstrZUnit)
{
    m_qstrZUnit = qstrZUnit;

    sigUpdateScalesAndLabels();
}

void cWaterfallQwtPlotWidget::addData(const QVector<float> &qvfYData, int64_t i64Timestamp_us)
{
    if(m_qvfAverage.size() != qvfYData.size())
    {
        m_qvfAverage.resize(qvfYData.size());

        //Reset the avarage if the vector length has changed
        for(uint32_t ui = 0; ui < (uint32_t)m_qvfAverage.size(); ui++)
        {
            m_qvfAverage[ui] = 0.0f;
        }
        m_u32AverageCount = 0;
    }

    //Accumulate
    for(uint32_t ui = 0; ui < (uint32_t)m_qvfAverage.size(); ui++)
    {
        m_qvfAverage[ui] += qvfYData[ui];
    }
    m_u32AverageCount++;

    //Use span as per set in the GUI to determine how long to average for before adding a new line.
    //Essentially number of rows * average time per row = span time
    if( i64Timestamp_us - m_pSpectrogramData->getMaxTime_us() < (int64_t)m_pTimeSpanSpinBox_s->value() * 1000000 / m_pSpectrogramData->getNRows() )
    {
        return;
    }

    //When it is time for a new line use the average
    for(uint32_t ui = 0; ui < (uint32_t)m_qvfAverage.size(); ui++)
    {
        m_qvfAverage[ui] /= m_u32AverageCount;
    }
    m_pSpectrogramData->addFrame(m_qvfAverage, i64Timestamp_us);

    //Reset the average
    for(uint32_t ui = 0; ui < (uint32_t)m_qvfAverage.size(); ui++)
    {
        m_qvfAverage[ui] = 0.0f;
    }
    m_u32AverageCount = 0;

    //Calculate the min and max plotting range of the spectrogram data
    float fMedian = m_pSpectrogramData->getMedian();
    double dDataMax, dDataMin;
    m_pSpectrogramData->getZMinMaxValue(dDataMin, dDataMax);

    if(m_bDoLogConversion)
    {
        float dMedian_dB = 10 * log10(fMedian);
        m_dZScaleMax = dMedian_dB + 20;
        m_dZScaleMin = dMedian_dB - 5;

        if(m_dZScaleMax > 10 * log10(dDataMax))
            m_dZScaleMax = 10 * log10(dDataMax);

        if(m_dZScaleMin < 10 * log10(dDataMin))
            m_dZScaleMin = 10 * log10(dDataMin);
    }
    else if(m_bDoPowerLogConversion)
    {
        float dMedian_dB = 20 * log10(fMedian);
        m_dZScaleMax = dMedian_dB + 20;
        m_dZScaleMin = dMedian_dB - 5;

        if(m_dZScaleMax > 20 * log10(dDataMax))
            m_dZScaleMax = 20 * log10(dDataMax);

        if(m_dZScaleMin < 20 * log10(dDataMin))
            m_dZScaleMin = 20 * log10(dDataMin);
    }
    else
    {
        float dMedian_dB = 10 * log10(fMedian);
        m_dZScaleMax = dMedian_dB * 100000;
        m_dZScaleMin = dMedian_dB - m_dZScaleMax;

        if(m_dZScaleMax > dDataMax)
            m_dZScaleMax = dDataMax;

        if(m_dZScaleMin < dDataMin)
            m_dZScaleMin = dDataMin;
    }

    if(!m_bIsPaused)
    {
        sigUpdateData();

        autoUpdateXScaleBase( qvfYData.size() );
    }
}

void cWaterfallQwtPlotWidget::setXRange(double dX1, double dX2)
{
    m_pUI->qwtPlot->setAxisScale(QwtPlot::xBottom, dX1, dX2);
    m_pSpectrogramData->setInterval(Qt::XAxis, QwtInterval(dX1, dX2));
}

void cWaterfallQwtPlotWidget::slotUpdateData()
{
    //Update the plot
    m_pPlotSpectrogram->setData(m_pSpectrogramData);

    if(m_bIsAutoscaleEnabled)
    {
        if(isfinite(m_dZScaleMin) && isfinite(m_dZScaleMax)) //Check for inf, nan etc.
        {
            setZRange(m_dZScaleMin, m_dZScaleMax);

            QWriteLocker oLock(&m_oMutex);
            m_bAutoscaleValid = true;
        }
        else
        {
            cout << "cWaterfallQwtPlotWidget::slotUpdateData(): Autoscale returned non-finite range [" << m_dZScaleMin << ", " << m_dZScaleMax << "] ignoring." << endl;

            QWriteLocker oLock(&m_oMutex);
            m_bAutoscaleValid = false;
        }

        m_pIntensityFloorSpinBox->blockSignals(true);
        m_pIntensityCeilingSpinBox->blockSignals(true);

        m_pIntensityFloorSpinBox->setValue(m_dZScaleMin);
        m_pIntensityCeilingSpinBox->setValue(m_dZScaleMax);

        m_pIntensityFloorSpinBox->blockSignals(false);
        m_pIntensityCeilingSpinBox->blockSignals(false);
    }

    m_pUI->qwtPlot->setAxisScale(QwtPlot::yLeft, m_pSpectrogramData->getMaxTime_us() / 1e6, m_pSpectrogramData->getMinTime_us() / 1e6);
}

void cWaterfallQwtPlotWidget::slotEnableAutoscale(bool bEnable)
{
    m_bIsAutoscaleEnabled = bEnable;

    m_pIntensityFloorSpinBox->setDisabled(bEnable);
    m_pIntensityCeilingSpinBox->setDisabled(bEnable);
}

void cWaterfallQwtPlotWidget::slotStrobeAutoscale(unsigned int u32Delay_ms)
{
    cout << "cWaterfallQwtPlotWidget::strobeAutoscale() strobing autoscale for plot " << m_qstrTitle.toStdString() << " in " << u32Delay_ms << " ms. Or until valid" << endl;

    //If autoscale is already on for this plot do nothing
    if(m_bIsAutoscaleEnabled)
        return;

    QTimer::singleShot(u32Delay_ms, this, SLOT(slotEnableAutoscale()));
    QTimer::singleShot(u32Delay_ms + 100, this, SLOT(slotDisableAutoscaleOnSuccess())); //Switch off 100 ms later
}

void cWaterfallQwtPlotWidget::slotDisableAutoscaleOnSuccess()
{
    QReadLocker oLock(&m_oMutex);

    if(m_bAutoscaleValid)
    {
        slotEnableAutoscale(false);
    }
    else
    {
        //Recheck for a valid autoscale in 100 millseconds time.
        QTimer::singleShot(100, this, SLOT(slotDisableAutoscaleOnSuccess()));
    }
}

void cWaterfallQwtPlotWidget::slotIntensityFloorChanged(double dValue)
{
    setZRange(dValue, m_pIntensityCeilingSpinBox->value());
}

void cWaterfallQwtPlotWidget::slotIntensityCeilingChanged(double dValue)
{
    setZRange(m_pIntensityFloorSpinBox->value(), dValue);
}

void cWaterfallQwtPlotWidget::setZRange(double dZMin, double dZMax)
{
    m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->setColorMap(QwtInterval(dZMin, dZMax), m_pColourMap);
    m_pUI->qwtPlot->setAxisScale(QwtPlot::yRight, dZMin, dZMax);
    m_pSpectrogramData->setInterval( Qt::ZAxis, QwtInterval(dZMin, dZMax ) );
}

void cWaterfallQwtPlotWidget::slotUpdateScalesAndLabels()
{
    cQwtPlotWidgetBase::slotUpdateScalesAndLabels();

    if(m_qstrZUnit.length())
    {
        QwtText oZLabel(QwtText(QString("%1 [%2]").arg(m_qstrZLabel).arg(m_qstrZUnit)) );
        oZLabel.setFont(m_oZFont);

        m_pIntensityCeilingSpinBox->setSuffix(QString(" %1").arg(m_qstrZUnit));
        m_pIntensityFloorSpinBox->setSuffix(QString(" %1").arg(m_qstrZUnit));

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yRight, oZLabel);
    }
    else
    {
        QwtText oZLabel( QwtText(QString("%1").arg(m_qstrZLabel)) );
        oZLabel.setFont(m_oZFont);

        m_pIntensityCeilingSpinBox->setSuffix(QString(""));
        m_pIntensityFloorSpinBox->setSuffix(QString(""));

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yRight, oZLabel);
    }
}

void cWaterfallQwtPlotWidget::enableLogConversion(bool bEnable)
{
    cQwtPlotWidgetBase::enableLogConversion(bEnable);

    m_pSpectrogramData->enableLogConversion(bEnable);
}

void cWaterfallQwtPlotWidget::enablePowerLogConversion(bool bEnable)
{
    cQwtPlotWidgetBase::enablePowerLogConversion(bEnable);

    m_pSpectrogramData->enablePowerLogConversion(bEnable);
}
