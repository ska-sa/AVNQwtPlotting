//System includes
#include <cmath>
#include <iostream>
#include <cfloat>

//Library includes

//Local includes
#include "ScrollingQwtLinePlotWidget.h"

using namespace std;

cScrollingQwtLinePlotWidget::cScrollingQwtLinePlotWidget(QWidget *pParent) :
    cBasicQwtLinePlotWidget(pParent),
    m_dSpanLength(120.0),
    m_dSpanLengthScalingFactor(1.0),
    m_dLastLogConversionXIndex(-DBL_MAX)
{
    //Add averaging control to GUI
    m_pSpanLengthLabel = new QLabel(QString("Span length"), this);
    m_pSpanLengthDoubleSpinBox = new QDoubleSpinBox(this);
    m_pSpanLengthDoubleSpinBox->setMinimum(1.0);
    m_pSpanLengthDoubleSpinBox->setMaximum(1000000.0);
    m_pSpanLengthDoubleSpinBox->setValue(120.0);
    insertWidgetIntoControlFrame(m_pSpanLengthLabel, 3);
    insertWidgetIntoControlFrame(m_pSpanLengthDoubleSpinBox, 4, true);

    QObject::connect(m_pSpanLengthDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(slotSetSpanLength(double)));
}

cScrollingQwtLinePlotWidget::~cScrollingQwtLinePlotWidget()
{
}

void cScrollingQwtLinePlotWidget::processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us)
{
    Q_UNUSED(i64Timestamp_us);

    //Add the input data to plot array
    for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)qvfXData.size(); u32SampleNo++)
    {
        m_qvdXDataToPlot.push_back(qvfXData[u32SampleNo]);
    }

    //Pop old data until the X span is correct
    while(m_qvdXDataToPlot.last() - m_qvdXDataToPlot.first() > m_dSpanLength * m_dSpanLengthScalingFactor)
    {
        m_qvdXDataToPlot.pop_front();
    }
}

void cScrollingQwtLinePlotWidget::processYData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us, const QVector<uint32_t> &qvu32ChannelList)
{
    Q_UNUSED(i64Timestamp_us);

    if(qvu32ChannelList.empty())
    {
        //Check that our output array has the right number of channels
        if(m_qvvdYDataToPlot.size() != qvvfYData.size())
        {
            m_qvvdYDataToPlot.resize(qvvfYData.size());
        }

        //Append new data
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)qvvfYData.size(); u32ChannelNo++)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)qvvfYData[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo].push_back(qvvfYData[u32ChannelNo][u32SampleNo]);
            }
        }

        //Pop data until the Y vector is the length as the X
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)qvvfYData.size(); u32ChannelNo++)
        {
            while(m_qvvdYDataToPlot[u32ChannelNo].size() > m_qvdXDataToPlot.size())
            {
                m_qvvdYDataToPlot[u32ChannelNo].pop_front();
            }
        }

    }
    else
    {
        //Check that our output array has the right number of channels
        if(m_qvvdYDataToPlot.size() != qvu32ChannelList.size())
        {
            m_qvvdYDataToPlot.resize(qvu32ChannelList.size());
        }

        //Append new data
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)qvu32ChannelList.size(); u32ChannelNo++)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)qvvfYData[qvu32ChannelList[u32ChannelNo]].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo].push_back(qvvfYData[qvu32ChannelList[u32ChannelNo]][u32SampleNo]);
            }
        }

        //Pop data until the Y vector is the length as the X
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)qvvfYData.size(); u32ChannelNo++)
        {
            while(m_qvvdYDataToPlot[u32ChannelNo].size() > m_qvdXDataToPlot.size())
            {
                m_qvvdYDataToPlot[u32ChannelNo].pop_front();
            }
        }
    }
    //cout << "cScrollingQwtLinePlotWidget::processXData(): m_qvdYDataToPlot is " << m_qvvdYDataToPlot[0].size() << " samples long." << endl;
}

void cScrollingQwtLinePlotWidget::logConversion()
{
    //Here we have to keep track of values already converted to dB.
    //This is done with the X value and stored in a member variable

    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
    {
        uint32_t u32SampleNo = 0;
        for(; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
        {
            if(m_qvdXDataToPlot[u32SampleNo] > m_dLastLogConversionXIndex)
            {
                break;
            }
        }

        for(; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
        {
            m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 10 * log10(m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] + 0.001);
        }
    }

    m_dLastLogConversionXIndex = m_qvdXDataToPlot.last();
}

void cScrollingQwtLinePlotWidget::powerLogConversion()
{
    //Here we have to keep track of values already converted to dB.
    //This is done with the X value and stored in a member variable

    for(uint32_t u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
    {
        uint32_t u32SampleNo = 0;
        for(; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
        {
            if(m_qvdXDataToPlot[u32SampleNo] >= m_dLastLogConversionXIndex)
            {
                cout << "Breaking at " << u32SampleNo << endl;
                break;
            }
        }

        for(; u32SampleNo <  (unsigned)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
        {
            m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 20 * log10(m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] + 0.001);
        }
    }

    m_dLastLogConversionXIndex = m_qvdXDataToPlot.last();
}

void cScrollingQwtLinePlotWidget::showSpanLengthControl(bool bEnable)
{
    m_pSpanLengthLabel->setVisible(bEnable);
    m_pSpanLengthLabel->setVisible(bEnable);
}

void cScrollingQwtLinePlotWidget::setSpanLengthControlScalingFactor(double dScalingFactor, const QString &qstrNewUnit)
{
    m_dSpanLengthScalingFactor = dScalingFactor;
    m_qstrSpanLengthSpinBoxUnitOveride = qstrNewUnit;

    sigUpdateScalesAndLabels();
}

void cScrollingQwtLinePlotWidget::slotSetSpanLength(double dSpanLength)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_dSpanLength = dSpanLength;
}

void cScrollingQwtLinePlotWidget::slotUpdateScalesAndLabels()
{
    //Do the updates that this function does in the base class
    cBasicQwtLinePlotWidget::slotUpdateScalesAndLabels();

    //With additional functionality:

    //If there is a scaling factor use the overriding unit.
    if(m_dSpanLengthScalingFactor != 1.0 || m_qstrSpanLengthSpinBoxUnitOveride.length())
        m_pSpanLengthDoubleSpinBox->setSuffix(QString(" %1").arg(m_qstrSpanLengthSpinBoxUnitOveride));
    else
        m_pSpanLengthDoubleSpinBox->setSuffix(QString(" %1").arg(m_qstrXUnit));
}
