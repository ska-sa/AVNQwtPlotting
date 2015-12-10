#ifndef WATERFALL_QWT_PLOT_WIDGET_H
#define WATERFALL_QWT_PLOT_WIDGET_H

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
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVector>
#include <qwt_plot_spectrogram.h>
#include <qwt_matrix_raster_data.h>
#include <qwt_plot_panner.h>
#include <qwt_interval.h>
#include <qwt_color_map.h>

//Local includes
#include "QwtPlotWidgetBase.h"
#include "WaterfallPlotSpectromgramData.h"
#include "QwtPlotPositionPicker.h"
#include "QwtPlotDistancePicker.h"
#include "CursorCentredQwtPlotMagnifier.h"
#include "AnimatedQwtPlotZoomer.h"
#include "WallTimeQwtScaleDraw.h"

namespace Ui {
class cWaterfallQwtPlotWidget;
}

class cWaterfallQwtPlotWidget : public cQwtPlotWidgetBase
{
    Q_OBJECT
    
public:
    explicit cWaterfallQwtPlotWidget(uint32_t u32ChannelNo, const QString &qstrChannelName, QWidget *pParent = 0);
    ~cWaterfallQwtPlotWidget();

    void                                setZLabel(const QString &qstrZLabel);
    void                                setZUnit(const QString &qstrZUnit);
    
    uint32_t                            getChannelNo(){return m_u32ChannelNo;}
    QString                             getChannelName(){return m_qstrChannelName;}
    
    void                                addData(const QVector<float> &qvfYData, int64_t i64Timestamp_us);

    void                                setXRange(double dX1, double dX2);
    
private:
    QwtPlotSpectrogram                  *m_pPlotSpectrogram;
    cWaterfallPlotSpectromgramData      *m_pSpectrogramData;
    QVector<float>                      m_qvfAverage;
    uint32_t                            m_u32AverageCount;

    //Addition plot settings
    QString                             m_qstrZLabel;
    QString                             m_qstrZUnit;

    QFont                               m_oZFont;

    //Qwt Plot extensions
    QwtLinearColorMap                   *m_pColourMap;

    //Custom Scale drawer
    cWallTimeQwtScaleDraw               *m_pTimeScaleDraw;

    //Addition controls
    QDoubleSpinBox                      *m_pIntensityCeilingSpinBox;
    QDoubleSpinBox                      *m_pIntensityFloorSpinBox;
    QLabel                              *m_pIntensityFloorLabel;
    QLabel                              *m_pIntensityCeilingLabel;

    QFont                               m_oTitleFont;
    QFont                               m_oXFont;
    QFont                               m_oYFont;

    uint32_t                            m_u32ChannelNo;
    QString                             m_qstrChannelName;

    double                              m_dZMin;
    double                              m_dZMax;

    void                                setZRange(double dZMin, double dZMax);
    
signals:
    void                                sigUpdateData();

public slots:
    virtual void                        slotEnableAutoscale(bool bEnable);
    void                                slotSetXSpan(double dStart, double dEnd);

protected slots:
    virtual void                        slotUpdateScalesAndLabels();
    void                                slotUpdateData();
    void                                slotIntensityFloorChanged(double dValue);
    void                                slotIntensityCeilingChanged(double dValue);

};

#endif // WATERFALL_QWT_PLOT_WIDGET_H
