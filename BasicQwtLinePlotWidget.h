#ifndef BASIC_QWT_LINE_PLOT_WIDGET_H
#define BASIC_QWT_LINE_PLOT_WIDGET_H

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
#include <QWidget>
#include <QString>
#include <QVector>
#include <QReadWriteLock>
#include <QFont>
#include <QTimer>
#include <QCheckBox>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_panner.h>
#include <qwt_interval.h>

//Local includes
#include "QwtPlotWidgetBase.h"
#include "CursorCentredQwtPlotMagnifier.h"
#include "AnimatedQwtPlotZoomer.h"

class cBasicQwtLinePlotWidget : public cQwtPlotWidgetBase
{
    Q_OBJECT

public:
    explicit cBasicQwtLinePlotWidget(QWidget *pParent = 0);
    virtual ~cBasicQwtLinePlotWidget();

    void                                addData(const QVector<float> &qvfXData, const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0,
                                                const QVector<uint32_t> &qvu32ChannelList = QVector<uint32_t>());

    void                                setCurveNames(const QVector<QString> &qvqstrCurveNames);

    void                                showPlotGrid(bool bEnable);

    QVector<Qt::GlobalColor>            m_qveCurveColours;

protected:
    //Addtional controls
    QCheckBox                           *m_pCheckBox_showLegend;

    //Curve related structures
    QVector<QwtPlotCurve*>              m_qvpPlotCurves;
    QVector<QString>                    m_qvqstrCurveNames;
    QVector<QwtPlotMarker*>             m_qvpVerticalLines;

    //Qwt Plot extensions
    QwtPlotGrid                         m_oPlotGrid;
    cAnimatedQwtPlotZoomer*             m_pPlotZoomer;
    QwtPlotPanner*                      m_pPlotPanner;
    cCursorCentredQwtPlotMagnifier*     m_pPlotMagnifier;

    //Data stuctures
    QVector<QVector<double> >           m_qvvdYDataToPlot;
    QVector<double>                     m_qvdXDataToPlot;
    int64_t                             m_i64PlotTimestamp_us;

    bool                                m_bIsGridShown;
    bool                                m_bShowVerticalLines;

    //Controls

    void                                showCurve(QwtPlotItem *pItem, bool bShow);

    virtual void                        processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us = 0);
    virtual void                        processYData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0, const QVector<uint32_t> &qvu32ChannelList = QVector<uint32_t>());

    virtual void                        logConversion();
    virtual void                        powerLogConversion();

    virtual void                        updateCurves();

public slots:
    virtual void                        slotEnableAutoscale(bool bEnable);
    void                                slotShowLegend(bool bEnable);
    void                                slotDrawVerticalLines(QVector<double> qvdXValues);
    void                                slotDrawVerticalLines(QVector<double> qvdXValues, QVector<QString> qvqstrLabels);
    void                                slotShowVerticalLines(bool bShow);

protected slots:
    virtual void                        slotUpdatePlotData();
    virtual void                        slotUpdateScalesAndLabels();
#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    void                                slotLegendChecked(QwtPlotItem *pPlotItem, bool bChecked);
#else
    void                                slotLegendChecked(const QVariant &oItemInfo, bool bChecked);
#endif

signals:
    void                                sigUpdatePlotData();

};

#endif // BASIC_QWT_LINE_PLOT_WIDGET_H
