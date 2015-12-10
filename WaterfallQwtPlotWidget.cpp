//System includes
#include <iostream>
#include <cmath>

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
    m_qstrChannelName(qstrChannelName)
{
    //Additional controls to the waterfall widget
    m_pIntensityFloorLabel = new QLabel(QString("Intensity floor"), this);
    m_pIntensityFloorSpinBox = new QDoubleSpinBox(this);
    m_pIntensityFloorSpinBox->setDecimals(0);
    m_pIntensityFloorSpinBox->setMaximum(200.0);
    m_pIntensityFloorSpinBox->setMinimum(-200.0);

    m_pIntensityCeilingLabel = new QLabel(QString("Intensity ceiling"), this);
    m_pIntensityCeilingSpinBox = new QDoubleSpinBox(this);
    m_pIntensityCeilingSpinBox->setDecimals(0);
    m_pIntensityCeilingSpinBox->setMaximum(200.0);
    m_pIntensityCeilingSpinBox->setMinimum(-200.0);

    insertWidgetIntoControlFrame(m_pIntensityFloorLabel, 3);
    insertWidgetIntoControlFrame(m_pIntensityFloorSpinBox, 4, true);
    insertWidgetIntoControlFrame(m_pIntensityCeilingLabel, 6);
    insertWidgetIntoControlFrame(m_pIntensityCeilingSpinBox, 7, true);

    m_pPlotSpectrogram = new QwtPlotSpectrogram;
    m_pSpectrogramData = new cWaterfallPlotSpectromgramData;

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
    m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->setTitle("Power Level [dB]");
    m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->setColorBarEnabled(true);
    m_pUI->qwtPlot->axisWidget(QwtPlot::yRight)->setColorMap(QwtInterval (50, 80), m_pColourMap);
    m_pUI->qwtPlot->setAxisScale(QwtPlot::yRight, 50, 80);
    m_pSpectrogramData->setInterval( Qt::ZAxis, QwtInterval(50.0, 80.0 ) );


    m_oZFont = m_pUI->qwtPlot->axisTitle(QwtPlot::yRight).font();
    m_oZFont.setPointSizeF(m_oZFont.pointSizeF() * 0.65);

    m_pUI->qwtPlot->setAutoReplot(true);
    m_pPlotSpectrogram->attach(m_pUI->qwtPlot);
    m_pPlotSpectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);

    m_pSpectrogramData->setDimensions(0, 200);

    //Install custom time scale drawer for the Y axis
    m_pUI->qwtPlot->setAxisScaleDraw(QwtPlot::yLeft, m_pTimeScaleDraw);

    QObject::connect(this, SIGNAL(sigUpdateData()), this, SLOT(slotUpdateData()), Qt::QueuedConnection);
    QObject::connect(m_pIntensityFloorSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotIntensityFloorChanged(double)) );
    QObject::connect(m_pIntensityCeilingSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotIntensityCeilingChanged(double)) );
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

    //Try to get approximately 10 lines per second
    if(i64Timestamp_us - m_pSpectrogramData->getMaxTime_us() < 100000)
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

    //Calculate the min and max of the spectrogram data
    m_pSpectrogramData->getZMinMaxValue(m_dZMin, m_dZMax);
    m_dZMax = 10 * log10(m_dZMax + 0.001);
    m_dZMin = 10 * log10(m_dZMin + 0.001);
    if(m_dZMax - m_dZMin > 100.0)
        m_dZMin = m_dZMax - 100.0;

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
        setZRange(m_dZMin, m_dZMax);

        m_pIntensityFloorSpinBox->blockSignals(true);
        m_pIntensityCeilingSpinBox->blockSignals(true);

        m_pIntensityFloorSpinBox->setValue(m_dZMin);
        m_pIntensityCeilingSpinBox->setValue(m_dZMax);

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

void cWaterfallQwtPlotWidget::slotSetXSpan(double dStart, double dEnd)
{
    m_pUI->qwtPlot->setAxisScale(QwtPlot::xBottom, dStart, dEnd);
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

        m_pIntensityCeilingSpinBox->setSuffix(QString(" %1").arg(m_qstrYUnit));
        m_pIntensityFloorSpinBox->setSuffix(QString(" %1").arg(m_qstrYUnit));

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yRight, oZLabel);
    }
    else
    {
        QwtText oZLabel( QwtText(QString("%1").arg(m_qstrZLabel)) );
        oZLabel.setFont(m_oZFont);

        m_pIntensityCeilingSpinBox->setSuffix(QString(""));
        m_pIntensityFloorSpinBox->setSuffix(QString(""));

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, oZLabel);
    }
}
