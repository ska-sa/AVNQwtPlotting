//System includes
#include <iostream>

//Library includes
#include <boost/math/special_functions/trunc.hpp>

//Local includes
#include "BandPowerQwtLinePlotWidget.h"
#include "ui_BasicQwtLinePlotWidget.h"

using namespace std;

cBandPowerQwtLinePlot::cBandPowerQwtLinePlot(QWidget *pParent) :
    cScrollingQwtLinePlotWidget(pParent),
    m_dBandMinimum(0),
    m_dBandMaximum(1),
    m_i64IntegrationStartTime_us(0),
    m_bNewIntegration(true)
{
    //Add averaging control to GUI
    m_pBandStartLabel = new QLabel(QString("Band start"), this);
    m_pBandStopLabel = new QLabel(QString("Band stop"), this);
    m_pBandStartDoubleSpinBox = new QDoubleSpinBox(this);
    m_pBandStopDoubleSpinBox = new QDoubleSpinBox(this);
    insertWidgetIntoControlFrame(m_pBandStartLabel, 8);
    insertWidgetIntoControlFrame(m_pBandStartDoubleSpinBox, 9, true);
    insertWidgetIntoControlFrame(m_pBandStopLabel, 11);
    insertWidgetIntoControlFrame(m_pBandStopDoubleSpinBox, 12, true);

    QObject::connect(m_pBandStartDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotBandStartChanged(double)));
    QObject::connect(m_pBandStopDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotBandStopChanged(double)));

    m_qvfIntergratedPowerTimestamp_s.resize(1);

    m_pUI->qwtPlot->setAxisScaleDraw(QwtPlot::xBottom, &m_oTimeScaleDraw);
}

cBandPowerQwtLinePlot::~cBandPowerQwtLinePlot()
{
}

void cBandPowerQwtLinePlot::addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us)
{
    //Get array indexes from selection frequency band
    uint32_t u32StartIndex = boost::math::itrunc(m_dSelectedBandStart / (m_dBandMaximum - m_dBandMinimum) * qvvfYData[0].size()); //trunc = floor
    uint32_t u32StopIndex = boost::math::itrunc(m_dSelectedBandStop / (m_dBandMaximum - m_dBandMinimum) * qvvfYData[0].size());

    //Check that we have correct number of channels
    if(m_qvvfIntergratedPower.size() != qvvfYData.size())
    {
        m_qvvfIntergratedPower.resize(qvvfYData.size());

        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvfIntergratedPower.size(); u32ChannelNo++)
        {
            m_qvvfIntergratedPower[u32ChannelNo].resize(1); //Only a single sample of integration output is added per iteration
        }
    }

    //Now for each channel sum the values over the specified frequency range
    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvfIntergratedPower.size(); u32ChannelNo++)
    {
        if(m_bNewIntegration)
        {
            m_qvvfIntergratedPower[u32ChannelNo][0] = 0.0; //Reset integrated power to 0
        }

        //Integrate
        for(uint32_t u32Index = u32StartIndex; u32Index <= u32StopIndex && u32Index < (uint32_t)qvvfYData[u32ChannelNo].size(); u32Index++)
        {
            m_qvvfIntergratedPower[u32ChannelNo][0] += qvvfYData[u32ChannelNo][u32Index];
        }
    }

    //Store the timestamp
    m_qvfIntergratedPowerTimestamp_s[0] = (i64Timestamp_us / 1000000) % (60 * 60 * 24);

    if(m_bNewIntegration)
    {
        m_i64IntegrationStartTime_us = i64Timestamp_us;
        m_bNewIntegration = false;
    }

    //Pass to the underlying plotting code
    if(i64Timestamp_us - m_i64IntegrationStartTime_us >= 1000000) //1 second
    {
        //Divide by bandwidth
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvfIntergratedPower.size(); u32ChannelNo++)
        {
             m_qvvfIntergratedPower[u32ChannelNo][0] /= (m_dBandMaximum - m_dBandMinimum);
        }
        cScrollingQwtLinePlotWidget::addData(m_qvfIntergratedPowerTimestamp_s, m_qvvfIntergratedPower, i64Timestamp_us);
        m_bNewIntegration = true;
    }
}

void cBandPowerQwtLinePlot::setSelectableBand(double dBandMinimum, double dBandMaximum, uint32_t u32NDiscreteFrequencies, const QString &qstrUnit)
{
    m_oMutex.lockForWrite();

    m_dBandMinimum= dBandMinimum;
    m_dBandMaximum = dBandMaximum;
    m_u32NDiscreteBandFreqencies = u32NDiscreteFrequencies;
    m_qstrBandUnit = qstrUnit;

    m_oMutex.unlock();

    sigUpdateScalesAndLabels();
}


void  cBandPowerQwtLinePlot::slotUpdateScalesAndLabels()
{
    //Do the updates that this function does in the parent class
    cScrollingQwtLinePlotWidget::slotUpdateScalesAndLabels();

    //With additional functionality:
    m_oMutex.lockForWrite();

    m_pBandStartDoubleSpinBox->setMinimum(m_dBandMinimum);
    m_pBandStartDoubleSpinBox->setMaximum(m_dBandMaximum);
    m_pBandStartDoubleSpinBox->setSingleStep( (m_dBandMaximum - m_dBandMinimum) / (m_u32NDiscreteBandFreqencies - 1) );
    m_pBandStartDoubleSpinBox->setSuffix(QString(" %1").arg(m_qstrBandUnit));

    m_pBandStopDoubleSpinBox->setMinimum(m_dBandMinimum);
    m_pBandStopDoubleSpinBox->setMaximum(m_dBandMaximum);
    m_pBandStopDoubleSpinBox->setSingleStep( (m_dBandMaximum - m_dBandMinimum) / (m_u32NDiscreteBandFreqencies - 1) );
    m_pBandStopDoubleSpinBox->setSuffix(QString(" %1").arg(m_qstrBandUnit));

    m_oMutex.unlock();
}

void cBandPowerQwtLinePlot::slotBandStartChanged(double dBandStart)
{
    m_oMutex.lockForWrite();

    m_dSelectedBandStart = dBandStart;

    if(m_dSelectedBandStart > m_dSelectedBandStop)
    {
        m_dSelectedBandStop = m_dSelectedBandStart;
        slotSetSelectedBandStop(m_dSelectedBandStop); //Update spin box
    }

    m_oMutex.unlock();

    sigSelectedBandChanged(m_dSelectedBandStart, m_dSelectedBandStop);

    QVector<double> qvdBandSelection;
    qvdBandSelection.push_back(m_dSelectedBandStart);
    qvdBandSelection.push_back(m_dSelectedBandStop);

    sigSelectedBandChanged(qvdBandSelection);
}

void cBandPowerQwtLinePlot::slotBandStopChanged(double dBandStop)
{
    m_oMutex.lockForWrite();

    m_dSelectedBandStop = dBandStop;

    if(m_dSelectedBandStart > m_dSelectedBandStop)
    {
        m_dSelectedBandStart = m_dSelectedBandStop;
        slotSetSelectedBandStart(m_dSelectedBandStop); //Update spin box
    }

    m_oMutex.unlock();

    sigSelectedBandChanged(m_dSelectedBandStart, m_dSelectedBandStop);

    QVector<double> qvdBandSelection;
    qvdBandSelection.push_back(m_dSelectedBandStart);
    qvdBandSelection.push_back(m_dSelectedBandStop);

    sigSelectedBandChanged(qvdBandSelection);
}

void cBandPowerQwtLinePlot::slotSetSelectedBandStart(double dBandStart)
{
    m_pBandStartDoubleSpinBox->blockSignals(true);

    m_pBandStartDoubleSpinBox->setValue(dBandStart);

    m_pBandStartDoubleSpinBox->blockSignals(false);
}

void cBandPowerQwtLinePlot::slotSetSelectedBandStop(double dBandStop)
{
    m_pBandStopDoubleSpinBox->blockSignals(true);

    m_pBandStopDoubleSpinBox->setValue(dBandStop);

    m_pBandStopDoubleSpinBox->blockSignals(false);
}

void cBandPowerQwtLinePlot::slotSetSelectedBand(double dBandStart, double dBandStop)
{
    slotSetSelectedBandStart(dBandStart);
    slotSetSelectedBandStop(dBandStop);
}
