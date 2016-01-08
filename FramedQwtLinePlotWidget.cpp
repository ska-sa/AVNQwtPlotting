//System includes
#include <cmath>
#include <iostream>

//Library includes
#include <qwt_scale_widget.h>

//Local includes
#include "FramedQwtLinePlotWidget.h"
#include "ui_QwtPlotWidgetBase.h"

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
    m_pAveragingSpinBox->setKeyboardTracking(false);
    insertWidgetIntoControlFrame(m_pAveragingLabel, 3);
    insertWidgetIntoControlFrame(m_pAveragingSpinBox, 4, true);

    //Add waterfall menu
    m_pWaterfallMenu = new QMenu(this);
    m_pWaterfallMenuButton = new QToolButton(this);
    m_pWaterfallMenuButton->setText(QString("Waterfall Plots"));
    m_pWaterfallMenuButton->setMenu(m_pWaterfallMenu);
    m_pWaterfallMenuButton->setPopupMode(QToolButton::InstantPopup);
    insertWidgetIntoControlFrame(m_pWaterfallMenuButton, 6, true);

    slotSetAverage(1);

    QObject::connect(m_pAveragingSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotSetAverage(int)));
    QObject::connect(m_pWaterfallMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotWaterFallPlotEnabled(QAction*)));
}

cFramedQwtLinePlotWidget::~cFramedQwtLinePlotWidget()
{
}

void cFramedQwtLinePlotWidget::addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us, const QVector<uint32_t> &qvu32ChannelList)
{
    //Pass the first channel of the Y data as the X array.
    //Only the length information is needed to update the X scale.
    cBasicQwtLinePlotWidget::addData(qvvfYData[0], qvvfYData, i64Timestamp_us, qvu32ChannelList);

    {
        QReadLocker oLock(&m_oWaterfallPlotMutex);

        //Also pass data to any existing waterfall plots
        for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size(); ui++)
        {
            m_qvpWaterfallPlots[ui]->addData(qvvfYData[ qvu32ChannelList[m_qvpWaterfallPlots[ui]->getChannelNo()] ], i64Timestamp_us);
        }
    }
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

    {
        QReadLocker oLock(&m_oWaterfallPlotMutex);

        for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size(); ui++)
        {
            cout << "Setting waterfall plot " << m_qvpWaterfallPlots[ui]->getChannelName().toStdString() << "X span to " << m_dXBegin << ", " << m_dXEnd << endl;
            m_qvpWaterfallPlots[ui]->setXRange(m_dXBegin, m_dXEnd);
        }
    }

    //Flag for ploting code to update
    m_bXSpanChanged = true;
}


void cFramedQwtLinePlotWidget::slotSetAverage(int iAveraging)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_u32Averaging = iAveraging;
}

void cFramedQwtLinePlotWidget::updateCurves()
{
    //Do what the base function does
    cBasicQwtLinePlotWidget::updateCurves();

    //But also populated the waterfall menu with the correct number of curves.
    if(m_pWaterfallMenu->actions().size() != m_qvvdYDataToPlot.size())
    {
        cout << "cFramedQwtLinePlotWidget::slotUpdateCurves() Updating to " << m_qvvdYDataToPlot.size() << " plot menu entries for waterfall plot " << m_qstrTitle.toStdString() << endl;

        removeAllWaterfallPlots();
        m_pWaterfallMenu->clear();

        //Create new entries
        for(unsigned int u32ChannelNo = 0; u32ChannelNo < (unsigned)m_qvvdYDataToPlot.size(); u32ChannelNo++)
        {
            cIndexedCheckableQAction *pAction;
            if(u32ChannelNo < (uint32_t)m_qvqstrCurveNames.size())
            {
                pAction = new cIndexedCheckableQAction(u32ChannelNo, m_qvqstrCurveNames[u32ChannelNo], this);
            }
            else
            {
                pAction = new cIndexedCheckableQAction(u32ChannelNo, QString("Channel %1").arg(u32ChannelNo), this);
            }

            m_pWaterfallMenu->addAction(pAction);
        }
    }
}

void cFramedQwtLinePlotWidget::slotWaterFallPlotEnabled(QAction* pAction)
{
    if(pAction->isChecked())
    {
        cIndexedCheckableQAction *pIndexAbleAction = qobject_cast<cIndexedCheckableQAction*>(pAction);

        if(pIndexAbleAction)
        {
            cout << "Creating new waterfall plot for curve " << pAction->text().toStdString() << endl;
            addWaterfallPlot(pIndexAbleAction->getIndex(), pAction->text());
        }
    }
    else
    {
        cout << "Removing waterfall plot for curve " << pAction->text().toStdString() << endl;
        removeWaterfallPlot(pAction->text());
    }
}

void cFramedQwtLinePlotWidget::addWaterfallPlot(uint32_t u32ChannelNo, const QString &qstrChannelName)
{
    cWaterfallQwtPlotWidget *pWaterfallPlot = new cWaterfallQwtPlotWidget(u32ChannelNo, qstrChannelName, this);

    {
        QWriteLocker oLock(&m_oWaterfallPlotMutex);
        m_qvpWaterfallPlots.push_back(pWaterfallPlot);
        m_pUI->verticalLayout->insertWidget(0, pWaterfallPlot);
    }

    {
        QReadLocker oLock(&m_oWaterfallPlotMutex);
        cout << "Setting waterfall plot " << pWaterfallPlot->getChannelName().toStdString() << " X-span to " << m_dXBegin << ", " << m_dXEnd << endl;
        pWaterfallPlot->setXRange(m_dXBegin, m_dXEnd);
        pWaterfallPlot->setXLabel(m_qstrXLabel);
        pWaterfallPlot->setXUnit(m_qstrXUnit);
        pWaterfallPlot->setYLabel(QString("Timestamp"));
        pWaterfallPlot->setYScaleIsTime(true);
        pWaterfallPlot->setZLabel(m_qstrYLabel);
        cout << "Setting label unit to " << m_qstrYLabel.toStdString() << endl;
        pWaterfallPlot->setZUnit(m_qstrYUnit);
        cout << "Setting Z unit to " << m_qstrYUnit.toStdString() << endl;
        pWaterfallPlot->setTitle(qstrChannelName);

        //Use the same scale dB / linear as the frame plot
        if(m_bDoLogConversion)
        {
            pWaterfallPlot->enableLogConversion(true);
        }
        else if(m_bDoPowerLogConversion)
        {
            pWaterfallPlot->enablePowerLogConversion(true);
        }
    }

    //Connect mouse position indicator of this framed plot and the derived waterfall plot together.
    QObject::connect(this, SIGNAL(sigSharedMousePositionChanged(QPointF,bool)), pWaterfallPlot, SLOT(slotUpdateSharedMouseHPosition(QPointF,bool)) );
    QObject::connect(pWaterfallPlot, SIGNAL(sigSharedMousePositionChanged(QPointF,bool)), this, SLOT(slotUpdateSharedMouseHPosition(QPointF,bool)) );

    slotScaleDivChanged();
}

void cFramedQwtLinePlotWidget::removeWaterfallPlot(const QString &qstrChannelName)
{
    QWriteLocker oLock(&m_oWaterfallPlotMutex);

    for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size();)
    {
        if(m_qvpWaterfallPlots[ui]->getChannelName() == qstrChannelName)
        {          
            //Disconnect mouse position indicator of this framed plot and the derived waterfall plot .
            QObject::disconnect(this, SIGNAL(sigSharedMousePositionChanged(QPointF,bool)), m_qvpWaterfallPlots[ui], SLOT(slotUpdateSharedMouseHPosition(QPointF,bool)) );
            QObject::disconnect(m_qvpWaterfallPlots[ui], SIGNAL(sigSharedMousePositionChanged(QPointF,bool)), this, SLOT(slotUpdateSharedMouseHPosition(QPointF,bool)) );

            delete m_qvpWaterfallPlots[ui];
            m_qvpWaterfallPlots.erase(m_qvpWaterfallPlots.begin() + ui);

            cout << "cFramedQwtLinePlotWidget::removeWaterfallPlot()): Deleted waterfall plot for curve: " << qstrChannelName.toStdString() << endl;
        }
        else
        {
            ui++;
        }
    }
}

void cFramedQwtLinePlotWidget::removeAllWaterfallPlots()
{
    QWriteLocker oLock(&m_oWaterfallPlotMutex);

    for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size();)
    {
        delete m_qvpWaterfallPlots[ui];
        m_qvpWaterfallPlots.erase(m_qvpWaterfallPlots.begin() + ui);
    }
}

void cFramedQwtLinePlotWidget::slotScaleDivChanged()
{
    cBasicQwtLinePlotWidget::slotScaleDivChanged();

    QReadLocker oLock(&m_oWaterfallPlotMutex);

    for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size(); ui++)
    {
        m_qvpWaterfallPlots[ui]->slotUpdateXScaleDiv(m_pUI->qwtPlot->axisInterval(QwtPlot::xBottom).minValue(), m_pUI->qwtPlot->axisInterval(QwtPlot::xBottom).maxValue());
    }
}

void cFramedQwtLinePlotWidget::slotUpdateScalesAndLabels()
{
    cBasicQwtLinePlotWidget::slotUpdateScalesAndLabels();

    QReadLocker oLock(&m_oWaterfallPlotMutex);

    //Check that all waterfall axes labels and units are up to date
    for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size(); ui++)
    {
        m_qvpWaterfallPlots[ui]->setXLabel(m_qstrXLabel);
        m_qvpWaterfallPlots[ui]->setXUnit(m_qstrXUnit);
        m_qvpWaterfallPlots[ui]->setZLabel(m_qstrYLabel);
        m_qvpWaterfallPlots[ui]->setZUnit(m_qstrYUnit);
    }
}

void cFramedQwtLinePlotWidget::slotStrobeAutoscale(unsigned int u32Delay_ms)
{
    cQwtPlotWidgetBase::slotStrobeAutoscale(u32Delay_ms);

    for(uint32_t ui = 0; ui < (uint32_t)m_qvpWaterfallPlots.size(); ui++)
    {
        m_qvpWaterfallPlots[ui]->slotStrobeAutoscale(u32Delay_ms);
    }
}
