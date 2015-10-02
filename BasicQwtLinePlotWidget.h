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
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_marker.h>

//Local includes
#include "BasicQwtLinePlotPicker.h"

namespace Ui {
class cBasicQwtLinePlotWidget;
}

class cBasicQwtLinePlotWidget : public QWidget
{
    Q_OBJECT

public:   

    explicit cBasicQwtLinePlotWidget(QWidget *pParent = 0);
    ~cBasicQwtLinePlotWidget();

    void                                addData(const QVector<float> &qvfXData, const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0);

    void                                setXLabel(const QString &qstrXLabel);
    void                                setXUnit(const QString &qstrXUnit);
    void                                setYLabel(const QString &qstrYLabel);
    void                                setYUnit(const QString &qstrYUnit);
    void                                setTitle(const QString &qstrTitle);
    void                                setCurveNames(const QVector<QString> &qvqstrCurveNames);

    void                                showPlotGrid(bool bEnable);

    void                                showAutoscaleControl(bool bEnable);
    void                                showPauseControl(bool bEnable);
    void                                showLegendControl(bool bEnable);

    void                                enableLogConversion(bool bEnable);
    void                                enablePowerLogConversion(bool bEnable);

    void                                enableRejectData(bool bEnable);

    void                                enableTimestampInTitle(bool bEnable);

    QVector<Qt::GlobalColor>            m_qveCurveColours;

protected:
    Ui::cBasicQwtLinePlotWidget         *m_pUI;

    QVector<QwtPlotCurve*>              m_qvpPlotCurves;
    QVector<QString>                    m_qvqstrCurveNames;
    QVector<QwtPlotMarker*>             m_qvpVerticalLines;

    //Qwt Plot extensions
    QwtPlotGrid                         m_oPlotGrid;
    QwtPlotZoomer*                      m_pPlotZoomer;
    QwtPlotPanner*                      m_pPlotPanner;
    QwtPlotMagnifier*                   m_pPlotMagnifier;
    cBasicQwtLinePlotPicker*            m_pPlotPicker;

    //Data stuctures
    QVector<QVector<double> >           m_qvvdYDataToPlot;
    QVector<double>                     m_qvdXDataToPlot;

    //Plot settings
    QString                             m_qstrTitle;
    QString                             m_qstrXLabel;
    QString                             m_qstrXUnit;
    QString                             m_qstrYLabel;
    QString                             m_qstrYUnit;

    QFont                               m_oTitleFont;
    QFont                               m_oXFont;
    QFont                               m_oYFont;

    bool                                m_bIsGridShown;
    bool                                m_bShowVerticalLines;

    //Controls
    bool                                m_bIsPaused;
    bool                                m_bIsAutoscaleEnabled;
    uint32_t                            m_u32Averaging;

    bool                                m_bTimestampInTitleEnabled;

    bool                                m_bDoLogConversion;
    bool                                m_bDoPowerLogConversion;

    bool                                m_bRejectData;

    QReadWriteLock                      m_oMutex;

    void                                connectSignalsToSlots();

    void                                showCurve(QwtPlotItem *pItem, bool bShow);
    void                                updateCurves();
    virtual void                        processXData(const QVector<float> &qvfXData, int64_t i64Timestamp_us = 0);
    virtual void                        processYData(const QVector<QVector<float> > &qvvfYData, int64_t i64Timestamp_us = 0);

    void                                insertWidgetIntoControlFrame(QWidget* pNewWidget, uint32_t u32Index, bool bAddSpacerAfter = false);

public slots:
    void                                slotPauseResume();
    void                                slotPause(bool bPause);
    void                                slotEnableAutoscale(bool bEnable);
    void                                slotShowLegend(bool bEnable);
    void                                slotDrawVerticalLines(QVector<double> qvdXValues);
    void                                slotDrawVerticalLines(QVector<double> qvdXValues, QVector<QString> qvqstrLabels);
    void                                slotShowVerticalLines(bool bShow);

protected slots:
    void                                slotUpdatePlotData(unsigned int uiCurveNo, QVector<double> qvdXData, QVector<double> qvdYData, int64_t i64Timestamp_us);
    virtual void                        slotUpdateScalesAndLabels();
    void                                slotUpdateXScaleBase(int iBase);
    void                                slotLegendChecked(const QVariant &oItemInfo, bool bChecked);
    void                                slotGrabFrame();

signals:
    void                                sigUpdatePlotData(unsigned int uiCurveNo, QVector<double> qvdXData, QVector<double> qvdYData, int64_t i64Timestamp_us);
    void                                sigUpdateScalesAndLabels();
    void                                sigUpdateXScaleBase(int iBase);

};

#endif // BASIC_QWT_LINE_PLOT_WIDGET_H
