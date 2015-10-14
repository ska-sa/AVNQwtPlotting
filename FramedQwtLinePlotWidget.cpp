//System includes
#include <cmath>
#include <iostream>

//Library includes

//Local includes
#include "FramedQwtLinePlotWidget.h"
#include "ui_BasicQwtLinePlotWidget.h"

using namespace std;

cFramedQwtLinePlotWidget::cFramedQwtLinePlotWidget(QWidget *pParent) :
    cBasicQwtLinePlotWidget(pParent),
    m_u32NextHistoryInputIndex(0),
    m_u32Averaging(1),
    m_dXBegin(0.0),
    m_dXEnd(1.0),
    m_bXSpanChanged(true)
{
    //Add averaging control to GUI
    m_pAveragingLabel = new QLabel(QString("Averaging"), this);
    m_pAveragingSpinBox = new QSpinBox(this);
    m_pAveragingSpinBox->setMinimum(1);
    m_pAveragingSpinBox->setMaximum(50);
    insertWidgetIntoControlFrame(m_pAveragingLabel, 3);
    insertWidgetIntoControlFrame(m_pAveragingSpinBox, 4, true);

    slotSetAverage(1);

    QObject::connect(m_pAveragingSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotSetAverage(int)));
}

cFramedQwtLinePlotWidget::~cFramedQwtLinePlotWidget()
{
}

void cFramedQwtLinePlotWidget::addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us, const QVector<uint32_t> &qvu32ChannelList)
{
    //Pass the first channel of the Y data as the X array.
    //Only the length information is needed to update the X scale.
    cBasicQwtLinePlotWidget::addData(qvvfYData[0], qvvfYData, i64Timestamp_us, qvu32ChannelList);
}

void cFramedQwtLinePlotWidget::processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us)
{
    Q_UNUSED(i64Timestamp_us);

    //Update X data
    //Generate our own scale based on the span member values. Only use the size of passed array. Not the data.

    m_oMutex.lockForRead(); //Ensure span doesn't change during this section

    if((uint32_t)m_qvdXDataToPlot.size() != (uint32_t)qvfXData.size() || m_bXSpanChanged)
    {
        m_qvdXDataToPlot.resize(qvfXData.size());

        double dInterval = (m_dXEnd - m_dXBegin) / (double)(m_qvdXDataToPlot.size() - 1);

        for(uint32_t u32XTick = 0; u32XTick < (uint32_t)m_qvdXDataToPlot.size(); u32XTick++)
        {
            m_qvdXDataToPlot[u32XTick] = m_dXBegin + u32XTick * dInterval;
        }

        m_bXSpanChanged = false;
    }

    m_oMutex.unlock();
}

void cFramedQwtLinePlotWidget::processYData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us, const QVector<uint32_t> &qvu32ChannelList)
{
    Q_UNUSED(i64Timestamp_us);

    if(qvu32ChannelList.empty())
    {
        //Check that our history and output array has the right number of channels
        if(m_qvvvfAverageHistory.size() != qvvfYData.size())
        {
            m_qvvvfAverageHistory.resize(qvvfYData.size());
            m_qvvdYDataToPlot.resize(qvvfYData.size());
        }

        //Update history length
        m_oMutex.lockForRead(); //Ensure averaging doesn't change during this section

        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvvfAverageHistory.size(); u32ChannelNo++)
        {
            //If shortening history simply delete the entries
            //Otherwise extend onces per sample update using the new history place to store the new data.
            //Simply resizing on enlarge would result in in signal level droppout until all entries have been used.
            if((uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size() > m_u32Averaging)
            {
                m_qvvvfAverageHistory[u32ChannelNo].resize(m_u32Averaging);
            }
            else if((uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size() < m_u32Averaging)
            {
                m_qvvvfAverageHistory[u32ChannelNo].resize(m_qvvvfAverageHistory[u32ChannelNo].size() + 1);
                m_u32NextHistoryInputIndex = m_qvvvfAverageHistory[u32ChannelNo].size() - 1;
            }

            //Also ensure that all new history frames are the same size as the input data
            for(uint32_t u32HistoryEntry = 0; u32HistoryEntry < (uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size(); u32HistoryEntry++)
            {
                m_qvvvfAverageHistory[u32ChannelNo][u32HistoryEntry].resize(qvvfYData[u32ChannelNo].size());
            }

            //Update number of samples in the array sent to the plotting widget too
            m_qvvdYDataToPlot[u32ChannelNo].resize(qvvfYData[u32ChannelNo].size());
        }

        m_oMutex.unlock();

        //Wrap history circular buffer as necessary
        if(m_u32NextHistoryInputIndex >= (uint32_t)m_qvvvfAverageHistory[0].size())
        {
            m_u32NextHistoryInputIndex = 0;
        }

        //Copy data into history
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvvfAverageHistory.size(); u32ChannelNo++)
        {
            std::copy(qvvfYData[u32ChannelNo].begin(), qvvfYData[u32ChannelNo].end(), m_qvvvfAverageHistory[u32ChannelNo][m_u32NextHistoryInputIndex].begin());
        }

        //Increment index for next data input
        m_u32NextHistoryInputIndex++;

        //Calculate Y data average to plot
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvdYDataToPlot.size(); u32ChannelNo++)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 0.0;

                //Sum
                for(uint32_t u32HistoryEntry = 0; u32HistoryEntry < (uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size(); u32HistoryEntry++)
                {
                    m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] += m_qvvvfAverageHistory[u32ChannelNo][u32HistoryEntry][u32SampleNo];
                }

                //Divide
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] /= m_qvvvfAverageHistory[u32ChannelNo].size();

                //cout << "m_qvvdYDataToPlot[" << u32ChannelNo << "][" << u32SampleNo << "] = " << m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] << endl;
            }
        }
    }
    else
    {
        //Check that our history and output array has the right number of channels
        if(m_qvvvfAverageHistory.size() != qvu32ChannelList.size())
        {
            m_qvvvfAverageHistory.resize(qvu32ChannelList.size());
            m_qvvdYDataToPlot.resize(qvu32ChannelList.size());
        }

        //Update history length
        m_oMutex.lockForRead(); //Ensure averaging doesn't change during this section

        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvvfAverageHistory.size(); u32ChannelNo++)
        {
            //If shortening history simply delete the entries
            //Otherwise extend onces per sample update using the new history place to store the new data.
            //Simply resizing on enlarge would result in in signal level droppout until all entries have been used.
            if((uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size() > m_u32Averaging)
            {
                m_qvvvfAverageHistory[u32ChannelNo].resize(m_u32Averaging);
            }
            else if((uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size() < m_u32Averaging)
            {
                m_qvvvfAverageHistory[u32ChannelNo].resize(m_qvvvfAverageHistory[u32ChannelNo].size() + 1);
                m_u32NextHistoryInputIndex = m_qvvvfAverageHistory[u32ChannelNo].size() - 1;
            }

            //Also ensure that all new history frames are the same size as the input data
            for(uint32_t u32HistoryEntry = 0; u32HistoryEntry < (uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size(); u32HistoryEntry++)
            {
                m_qvvvfAverageHistory[u32ChannelNo][u32HistoryEntry].resize(qvvfYData[qvu32ChannelList[u32ChannelNo]].size());
            }

            //Update number of samples in the array sent to the plotting widget too
            m_qvvdYDataToPlot[u32ChannelNo].resize(qvvfYData[qvu32ChannelList[u32ChannelNo]].size());
        }

        m_oMutex.unlock();

        //Wrap history circular buffer as necessary
        if(m_u32NextHistoryInputIndex >= (uint32_t)m_qvvvfAverageHistory[0].size())
        {
            m_u32NextHistoryInputIndex = 0;
        }

        //Copy data into history
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvvfAverageHistory.size(); u32ChannelNo++)
        {
            std::copy(qvvfYData[qvu32ChannelList[u32ChannelNo]].begin(), qvvfYData[qvu32ChannelList[u32ChannelNo]].end(), m_qvvvfAverageHistory[u32ChannelNo][m_u32NextHistoryInputIndex].begin());
        }

        //Increment index for next data input
        m_u32NextHistoryInputIndex++;

        //Calculate Y data average to plot
        for(uint32_t u32ChannelNo = 0; u32ChannelNo < (uint32_t)m_qvvdYDataToPlot.size(); u32ChannelNo++)
        {
            for(uint32_t u32SampleNo = 0; u32SampleNo < (uint32_t)m_qvvdYDataToPlot[u32ChannelNo].size(); u32SampleNo++)
            {
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] = 0.0;

                //Sum
                for(uint32_t u32HistoryEntry = 0; u32HistoryEntry < (uint32_t)m_qvvvfAverageHistory[u32ChannelNo].size(); u32HistoryEntry++)
                {
                    m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] += m_qvvvfAverageHistory[u32ChannelNo][u32HistoryEntry][u32SampleNo];
                }

                //Divide
                m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] /= m_qvvvfAverageHistory[u32ChannelNo].size();

                //cout << "m_qvvdYDataToPlot[" << u32ChannelNo << "][" << u32SampleNo << "] = " << m_qvvdYDataToPlot[u32ChannelNo][u32SampleNo] << endl;
            }
        }
    }
}

void cFramedQwtLinePlotWidget::showAveragingControl(bool bEnable)
{
    m_pAveragingSpinBox->setVisible(bEnable);
    m_pAveragingLabel->setVisible(bEnable);
}

void cFramedQwtLinePlotWidget::setXSpan(double dXBegin, double dXEnd)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_dXBegin = dXBegin;
    m_dXEnd = dXEnd;

    //Flag for ploting code to update
    m_bXSpanChanged = true;
}


void cFramedQwtLinePlotWidget::slotSetAverage(int iAveraging)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_u32Averaging = iAveraging;
}
