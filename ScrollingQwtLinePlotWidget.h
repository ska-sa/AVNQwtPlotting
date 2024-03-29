//Offers plotting for scrolling type data such as time domain data.

#ifndef SCROLLING_QWT_LINE_PLOT_WIDGET_H
#define SCROLLING_QWT_LINE_PLOT_WIDGET_H

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

//Local includes
#include "BasicQwtLinePlotWidget.h"

class cScrollingQwtLinePlotWidget : public cBasicQwtLinePlotWidget
{
    Q_OBJECT

public:   

    explicit cScrollingQwtLinePlotWidget(QWidget *pParent = 0);
    virtual ~cScrollingQwtLinePlotWidget();

    void                                showSpanLengthControl(bool bEnable);
    void                                setSpanLengthControlScalingFactor(double dScalingFactor, const QString &qstrNewUnit);

    void                                resetHistory();

protected:
    //GUI Widgets
    QDoubleSpinBox                      *m_pSpanLengthDoubleSpinBox;
    QLabel                              *m_pSpanLengthLabel;

    //Controls
    double                              m_dSpanLength;
    double                              m_dSpanLengthScalingFactor;
    QString                             m_qstrSpanLengthSpinBoxUnitOveride;

    double                              m_dPreviousLogConversionXIndex;
    double                              m_dPreviousOldestXSample;
    double                              m_dPreviousNewestXSample;

    virtual void                        processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us = 0);
    virtual void                        processYData(const QVector<QVector<float> > &qvvfXData, int64_t i64Timestamp_us = 0, const QVector<uint32_t> &qvu32ChannelList = QVector<uint32_t>());

    virtual void                        logConversion();
    virtual void                        powerLogConversion();

protected slots:
    virtual void                        slotUpdatePlotData();
    virtual void                        slotUpdateScalesAndLabels();

public slots:
    void                                slotSetSpanLength(double dSpanLength);

};

#endif // SCROLLING_QWT_LINE_PLOT_WIDGET_H
