#ifndef QWT_PLOT_WIDGET_BASE_H
#define QWT_PLOT_WIDGET_BASE_H

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
#include <QReadWriteLock>
#include <QFont>
#include <QTimer>
#include <qwt_interval.h>

//Local includes
#include "QwtPlotPositionPicker.h"
#include "QwtPlotDistancePicker.h"

namespace Ui {
class cQwtPlotWidgetBase;
}

class cQwtPlotWidgetBase : public QWidget
{
    Q_OBJECT

public:
    explicit cQwtPlotWidgetBase(QWidget *pParent = 0);
    virtual ~cQwtPlotWidgetBase();

    void                                setXLabel(const QString &qstrXLabel);
    void                                setXUnit(const QString &qstrXUnit);
    void                                setYLabel(const QString &qstrYLabel);
    void                                setXScaleIsTime(bool bXIsTime);
    void                                setYUnit(const QString &qstrYUnit);
    void                                setTitle(const QString &qstrTitle);
    void                                setYScaleIsTime(bool bYIsTime);

    void                                showAutoscaleControl(bool bEnable);
    void                                showPauseControl(bool bEnable);

    void                                enableLogConversion(bool bEnable);
    void                                enablePowerLogConversion(bool bEnable);

    void                                enableRejectData(bool bEnable);

    void                                enableTimestampInTitle(bool bEnable);

    void                                strobeAutoscale(uint32_t u32Delay_ms = 100);

    void                                autoUpdateXScaleBase(uint32_t u32NBins); //Sets the X scale to base 2 ticks if the number of bins is a power of 2

protected:
    Ui::cQwtPlotWidgetBase              *m_pUI;

    //Qwt Plot extensions
    cQwtPlotPositionPicker*             m_pPlotPositionPicker;
    cQwtPlotDistancePicker*             m_pPlotDistancePicker;

    //Plot settings
    QString                             m_qstrTitle;
    QString                             m_qstrXLabel;
    QString                             m_qstrXUnit;
    QString                             m_qstrYLabel;
    QString                             m_qstrYUnit;

    QFont                               m_oTitleFont;
    QFont                               m_oXFont;
    QFont                               m_oYFont;

    //Controls
    bool                                m_bIsPaused;
    bool                                m_bIsAutoscaleEnabled;
    QwtInterval                         m_oAutoscaledRange;

    bool                                m_bTimestampInTitleEnabled;

    bool                                m_bDoLogConversion;
    bool                                m_bDoPowerLogConversion;

    bool                                m_bRejectData;

    QReadWriteLock                      m_oMutex;

    void                                insertWidgetIntoControlFrame(QWidget* pNewWidget, uint32_t u32Index, bool bAddSpacerAfter = false);

public slots:
    void                                slotPauseResume();
    void                                slotPause(bool bPause);
    virtual void                        slotEnableAutoscale(bool bEnable) = 0;
    void                                slotEnableAutoscale();
    void                                slotDisableAutoscale();
    void                                slotStrobeAutoscale(unsigned int u32Delay_ms);

protected slots:
    virtual void                        slotUpdateScalesAndLabels();
    void                                slotSetXScaleBase(int iBase);
    void                                slotGrabFrame();

signals:
    void                                sigUpdateScalesAndLabels();
    void                                sigSetXScaleBase(int iBase);
    void                                sigStrobeAutoscale(unsigned int u32Delay_ms);

};

#endif // QWT_PLOT_WIDGET_BASE_H
