//System includes
#include <iostream>
#include <cmath>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/math/special_functions/trunc.hpp>
#endif

//Local includes
#include "BandPowerQwtLinePlotWidget.h"
#include "ui_QwtPlotWidgetBase.h"

using namespace std;

cBandPowerQwtLinePlot::cBandPowerQwtLinePlot(QWidget *pParent) :
    cScrollingQwtLinePlotWidget(pParent),
    m_pBandStartLabel(new QLabel(QString("Band start"), this)),
    m_pBandStopLabel(new QLabel(QString("Band stop"), this)),
    m_pIntegrationTimeLabel(new QLabel(QString("Integration time"), this)),
    m_pBandStartDoubleSpinBox(new QDoubleSpinBox(this)),
    m_pBandStopDoubleSpinBox(new QDoubleSpinBox(this)),
    m_pIntegrationTimeSpinBox(new QDoubleSpinBox(this)),
    m_pTimeScaleDraw(new cWallTimeQwtScaleDraw),
    m_dBandMinimum(0),
    m_dBandMaximum(1),
    m_dIntegrationTimeScalingFactor_s(1.0),
    m_dMaxIntegrationTime(60.0 * 60.0), //default to an our assuming default unit seconds.
    m_qstrIntegrationTimeUnit(QString("s")),
    m_dSelectedBandStart(0),
    m_dSelectedBandStop(1),
    m_i64IntegrationStartTime_us(0),
    m_i64IntegrationTime_us(1000000), //default 1 second
    m_bNewIntegration(true)
{
    //Only have the spinboxes emit a "changed" signal on Enter press. Not as the user types.
    m_pBandStartDoubleSpinBox->setKeyboardTracking(false);
    m_pBandStopDoubleSpinBox->setKeyboardTracking(false);
    m_pIntegrationTimeSpinBox->setKeyboardTracking(false);

    m_pIntegrationTimeSpinBox->setMinimum(0.0);
    m_pIntegrationTimeSpinBox->setValue(1.0);

    //Add band selection and intergration time selection to the GUI
    insertWidgetIntoControlFrame(m_pBandStartLabel, 6);
    insertWidgetIntoControlFrame(m_pBandStartDoubleSpinBox, 7, true);
    insertWidgetIntoControlFrame(m_pBandStopLabel, 9);
    insertWidgetIntoControlFrame(m_pBandStopDoubleSpinBox, 10, true);
    insertWidgetIntoControlFrame(m_pIntegrationTimeLabel, 12);
    insertWidgetIntoControlFrame(m_pIntegrationTimeSpinBox, 13, true);


    QObject::connect(m_pBandStartDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotBandStartChanged(double)));
    QObject::connect(m_pBandStopDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotBandStopChanged(double)));
    QObject::connect(m_pIntegrationTimeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotIntegrationTimeChanged(double)));

    m_qvfIntergratedPowerTimestamp_s.resize(1);

    m_pUI->qwtPlot->setAxisScaleDraw(QwtPlot::xBottom, m_pTimeScaleDraw);

    setXScaleIsTime(true); //Always true for this plot
}

cBandPowerQwtLinePlot::~cBandPowerQwtLinePlot()
{
}

void cBandPowerQwtLinePlot::addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us)
{

    double dSelectedBandWidth = m_dSelectedBandStop - m_dSelectedBandStart;

    //Prevent divide by zero
    if(dSelectedBandWidth == 0.0)
        dSelectedBandWidth = 1.0;

    //Get array indexes from selection frequency band
    uint32_t u32StartIndex = boost::math::itrunc( (m_dSelectedBandStart - m_dBandMinimum) / (m_dBandMaximum - m_dBandMinimum) * qvvfYData[0].size()); //trunc = floor
    uint32_t u32StopIndex = boost::math::itrunc( (m_dSelectedBandStop - m_dBandMinimum) / (m_dBandMaximum - m_dBandMinimum) * qvvfYData[0].size());

    //cout << "Start index = " << u32StartIndex << ", stop index = " << u32StopIndex << endl;

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
            m_qvvfIntergratedPower[u32ChannelNo][0] += std::fabs( qvvfYData[u32ChannelNo][u32Index] );
        }
    }

    if(m_bNewIntegration)
    {
        m_i64IntegrationStartTime_us = i64Timestamp_us;
        m_bNewIntegration = false;
    }

    //Pass to the underlying plotting code
    if(i64Timestamp_us - m_i64IntegrationStartTime_us >= m_i64IntegrationTime_us)
    {
        //Store the timestamp of the last spectrum frame in a float containing "seconds-elapsed-today" along with the bandpower.
        //This is the X axis for plotting

        m_qvfIntergratedPowerTimestamp_s[0] = fmod( (double)i64Timestamp_us / 1e6, 60 * 60 * 24 );

        //Divide by bandwidth
        //cout << "Selected bandwidth is " << dSelectedBandWidth << " " << m_qstrXUnit.toStdString() << endl;
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvfIntergratedPower.size(); u32ChannelNo++)
        {
            m_qvvfIntergratedPower[u32ChannelNo][0] /= dSelectedBandWidth;
            //cout << "Channel " << u32ChannelNo << " band power: " << m_qvvfIntergratedPower[u32ChannelNo][0] << endl;
        }
        cScrollingQwtLinePlotWidget::addData(m_qvfIntergratedPowerTimestamp_s, m_qvvfIntergratedPower, i64Timestamp_us);
        m_bNewIntegration = true;
    }
}

void cBandPowerQwtLinePlot::setSelectableBand(double dBandMinimum, double dBandMaximum, const QString &qstrUnit)
{
    m_oMutex.lockForWrite();

    m_dBandMinimum= dBandMinimum;
    m_dBandMaximum = dBandMaximum;
    m_qstrBandUnit = qstrUnit;

    m_oMutex.unlock();

    sigUpdateScalesAndLabels();
}

void cBandPowerQwtLinePlot::setIntegrationTimeControlScalingFactor(double dScalingFactor_s, const QString &qstrNewUnit, double dMaxSpinBoxValue)
{
    m_oMutex.lockForWrite();

    m_dIntegrationTimeScalingFactor_s = dScalingFactor_s;
    m_qstrIntegrationTimeUnit = qstrNewUnit;
    m_dMaxIntegrationTime = dMaxSpinBoxValue;

    m_oMutex.unlock();

    sigUpdateScalesAndLabels();
}


void  cBandPowerQwtLinePlot::slotUpdateScalesAndLabels()
{
    //Do the updates that this function does in the parent class
    cScrollingQwtLinePlotWidget::slotUpdateScalesAndLabels();

    //With additional functionality:

    m_pBandStartDoubleSpinBox->setMinimum(m_dBandMinimum);
    m_pBandStartDoubleSpinBox->setMaximum(m_dBandMaximum);
    m_pBandStartDoubleSpinBox->setSuffix(QString(" %1").arg(m_qstrBandUnit));

    m_pBandStopDoubleSpinBox->setMinimum(m_dBandMinimum);
    m_pBandStopDoubleSpinBox->setMaximum(m_dBandMaximum);
    m_pBandStopDoubleSpinBox->setSuffix(QString(" %1").arg(m_qstrBandUnit));

    m_pIntegrationTimeSpinBox->setSuffix(m_qstrIntegrationTimeUnit);
    m_pIntegrationTimeSpinBox->setMaximum(m_dMaxIntegrationTime);

    //Note: outside mutex to prevent recursive lock.
    //This is just a convenient initial state of setting initial selected band to maximum span so not functionaly critical.
    m_pBandStartDoubleSpinBox->setValue(m_dBandMinimum);
    m_pBandStopDoubleSpinBox->setValue(m_dBandMaximum);
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

void cBandPowerQwtLinePlot::slotIntegrationTimeChanged(double dIntegrationTime)
{
    m_oMutex.lockForWrite();

    m_i64IntegrationTime_us = dIntegrationTime * m_dIntegrationTimeScalingFactor_s * 1e6;

    m_oMutex.unlock();
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

