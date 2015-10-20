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
#include <qwt_scale_draw.h>

//Local includes
#include "ScrollingQwtLinePlotWidget.h"
#include "../../AVNUtilLibs/Timestamp/Timestamp.h"

//A derived Qwt class to draw custom time labels on the QwtPlot X axis
class cTimeScaleDraw : public QwtScaleDraw
{
public:
    cTimeScaleDraw()
    {
    }

    virtual QwtText label(double dValue_s) const
    {
        //The supplied value is whole seconds elapsed today
        //Print a HH:mm:ss label

        return QwtText( QString(AVN::stringFromTimestamp_HHmmss((int64_t)(dValue_s * 1e6)).c_str()) );
    }
};

class cBandPowerQwtLinePlot: public cScrollingQwtLinePlotWidget
{
    Q_OBJECT

public:   

    explicit cBandPowerQwtLinePlot(QWidget *pParent = 0);
    virtual ~cBandPowerQwtLinePlot();

    //Add data should take an array containing the full selectable band as specified by the function below
    virtual void                        addData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0);

    void                                setSelectableBand(double dBandMinimum, double dBandMaximum, uint32_t u32NDiscreteFrequencies, const QString &qstrUnit);
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
    cTimeScaleDraw                      *m_pTimeScaleDraw;

    //Settings
    double                              m_dBandMinimum;
    double                              m_dBandMaximum;
    uint32_t                            m_u32NDiscreteBandFreqencies;
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
    void                                slotSetSelectedBandStart(double dBandStart);
    void                                slotSetSelectedBandStop(double dBandStop);
    void                                slotSetSelectedBand(double dBandStart, double dBandStop);

signals:
    void                                sigSelectedBandChanged(double dBandStart, double dBandStop);
    void                                sigSelectedBandChanged(QVector<double> qvdSelection);
};

#endif // INTERGRATED_BANDWIDTH_QWT_LINE_PLOT_WIDGET_H
