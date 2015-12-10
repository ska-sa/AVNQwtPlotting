//Offers plotting for framed type data such as FFT, stokes etc.
//To this end data averaging is provided

#ifndef FRAMED_QWT_LINE_PLOT_WIDGET_H
#define FRAMED_QWT_LINE_PLOT_WIDGET_H

//System includes
#ifdef _WIN32
#include <stdint.h>

#ifndef int64_t
typedef __int64 int64_t;
#endif

#ifndef uint64_t
typedef unsigned __int64 uint64_t;
#endif

#else
#include <inttypes.h>
#endif

//Library includes
#include <QSpinBox>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QAction>

//Local includes
#include "BasicQwtLinePlotWidget.h"
#include "WaterfallQwtPlotWidget.h"

class cIndexedCheckableQAction : public QAction
{
    Q_OBJECT

public:
    explicit cIndexedCheckableQAction(uint32_t u32Index, const QString &qstrName, QWidget *pParent) :
        QAction(qstrName, pParent),
        m_u32Index(u32Index)
    {
        setCheckable(true);
    }

    uint32_t getIndex(){return m_u32Index;}

private:
    uint32_t m_u32Index;
};

class cFramedQwtLinePlotWidget : public cBasicQwtLinePlotWidget
{
    Q_OBJECT

public:   
    explicit cFramedQwtLinePlotWidget(QWidget *pParent = 0);
    virtual ~cFramedQwtLinePlotWidget();

    void                                addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0, const QVector<uint32_t> &qvu32ChannelList = QVector<uint32_t>());

    void                                setXSpan(double dXBegin, double dXEnd);

    void                                showAveragingControl(bool bEnable);

protected:
    //GUI Widgets
    QSpinBox                            *m_pAveragingSpinBox;
    QLabel                              *m_pAveragingLabel;
    QToolButton                         *m_pWaterfallMenuButton;
    QMenu                               *m_pWaterfallMenu;

    //Data stuctures
    QVector<QVector<QVector<float> >  > m_qvvvfAverageHistory;
    uint32_t                            m_u32NextHistoryInputIndex;

    //Controls
    uint32_t                            m_u32Averaging;

    //Span of X scale
    double                              m_dXBegin;
    double                              m_dXEnd;
    bool                                m_bXSpanChanged;

    //Callback handling
    QVector<cWaterfallQwtPlotWidget*>   m_qvpWaterfallPlots;
    QReadWriteLock                      m_oWaterfallPlotMutex;
    void                                addWaterfallPlot(uint32_t u32ChannelNo, const QString &qstrChannelName);
    void                                removeWaterfallPlot(const QString &qstrChannelName);

    virtual void                        updateCurves();

    virtual void                        processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us = 0);
    virtual void                        processYData(const QVector<QVector<float> > &qvvfXData, int64_t i64Timestamp_us = 0, const QVector<uint32_t> &qvu32ChannelList = QVector<uint32_t>());

public slots:
    void                                slotSetAverage(int iAveraging);

private slots:
    virtual void                        slotUpdateScalesAndLabels();
    void                                slotWaterFallPlotEnabled(QAction* pAction);
    void                                slotUpdateWaterPlotXScale();

};

#endif // FRAMED_QWT_LINE_PLOT_WIDGET_H
