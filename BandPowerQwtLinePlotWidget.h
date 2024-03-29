//A specific implementation of the scrolling line plot that is used for plotting power over a selected band.
//This will normally be paired with an framed line plot of an FFT.

#ifndef BAND_POWER_QWT_LINE_PLOT_WIDGET_H
#define BAND_POWER_QWT_LINE_PLOT_WIDGET_H

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
#include "ScrollingQwtLinePlotWidget.h"
#include "WallTimeQwtScaleDraw.h"

class cBandPowerQwtLinePlot: public cScrollingQwtLinePlotWidget
{
    Q_OBJECT

public:   

    explicit cBandPowerQwtLinePlot(QWidget *pParent = 0);
    virtual ~cBandPowerQwtLinePlot();

    //Add data should take an array containing the full selectable band as specified by the function below
    virtual void                        addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0, const QVector<uint32_t> &qvu32ChannelList = QVector<uint32_t>());

    void                                setSelectableBand(double dBandMinimum, double dBandMaximum, const QString &qstrUnit);
    void                                setIntegrationTimeControlScalingFactor(double dScalingFactor_s, const QString &qstrNewUnit, double dMaxSpinBoxValue);

protected:
    //GUI Widgets
    QLabel                              *m_pBandStartLabel;
    QLabel                              *m_pBandStopLabel;
    QLabel                              *m_pIntegrationTimeLabel;

    QDoubleSpinBox                      *m_pBandStartDoubleSpinBox;
    QDoubleSpinBox                      *m_pBandStopDoubleSpinBox;
    QDoubleSpinBox                      *m_pIntegrationTimeSpinBox;

    //Custom Scale drawer
    cWallTimeQwtScaleDraw               *m_pTimeScaleDraw;

    //Settings
    double                              m_dBandMinimum;
    double                              m_dBandMaximum;
    QString                             m_qstrBandUnit;
    double                              m_dIntegrationTimeScalingFactor_s;
    double                              m_dMaxIntegrationTime;
    QString                             m_qstrIntegrationTimeUnit;

    //Run time values
    double                              m_dSelectedBandStart;
    double                              m_dSelectedBandStop;

    QVector<QVector<float> >           m_qvvfIntergratedPower;
    QVector<float>                     m_qvfIntergratedPowerTimestamp_s;

    int64_t                            m_i64IntegrationStartTime_us;
    int64_t                            m_i64IntegrationTime_us;
    bool                               m_bNewIntegration;

protected slots:
    virtual void                        slotUpdateScalesAndLabels();

    void                                slotBandStartChanged(double dBandStart);
    void                                slotBandStopChanged(double dBandStop);

    void                                slotIntegrationTimeChanged(double dIntegrationTime);

public slots:
    //These change only the spin box value and do not emit subsequent signals
    void                                slotSetSelectedBandStart(double dBandStart);
    void                                slotSetSelectedBandStop(double dBandStop);
    void                                slotSetSelectedBand(double dBandStart, double dBandStop);

signals:
    void                                sigSelectedBandChanged(double dBandStart, double dBandStop);
    void                                sigSelectedBandChanged(QVector<double> qvdSelection);
};

#endif // INTERGRATED_BANDWIDTH_QWT_LINE_PLOT_WIDGET_H
