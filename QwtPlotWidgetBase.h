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
#include <QMainWindow>
#include <QString>
#include <QReadWriteLock>
#include <QFont>
#include <QTimer>
#include <qwt_interval.h>
#include <qwt_plot_marker.h>

//Local includes
#include "QwtPlotPositionPicker.h"
#include "QwtPlotDistancePicker.h"

namespace Ui {
class cQwtPlotWidgetBase;
}

class cQwtPlotWidgetBase : public QMainWindow //Plotting widget contains QDockWidget. It must therefore be of type QMainWindow to allow docking to work correctly (Qt caveat).
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

    void                                strobeAutoscale(uint32_t u32Delay_ms = 500);

    void                                autoUpdateXScaleBase(uint32_t u32NBins); //Sets the X scale to base 2 ticks if the number of bins is a power of 2

protected:
    Ui::cQwtPlotWidgetBase              *m_pUI;

    //Qwt Plot extensions
    cQwtPlotPositionPicker*             m_pPlotPositionPicker; //For showing current cursor position in text
    cQwtPlotDistancePicker*             m_pPlotDistancePicker; //For showing current selection dimensions with rectangle rubber band and text

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

    //Shared mouse position
    bool                                m_bMousePositionValid; //For sending

    QwtPlotMarker*                      m_pVSharedMousePosition;
    QwtPlotMarker*                      m_pHSharedMousePosition;
    bool                                m_bVSharedMousePositionValid; //For receiving
    bool                                m_bHSharedMousePositionValid;


    void                                insertWidgetIntoControlFrame(QWidget* pNewWidget, uint32_t u32Index, bool bAddSpacerAfter = false);

public slots:
    void                                slotPauseResume();
    void                                slotPause(bool bPause);
    virtual void                        slotEnableAutoscale(bool bEnable) = 0;
    void                                slotEnableAutoscale();
    void                                slotDisableAutoscale();
    virtual void                        slotStrobeAutoscale(unsigned int u32Delay_ms);
    void                                slotUpdateXScaleDiv(double dMin, double dMax);
    void                                slotUpdateSharedMousePosition(const QPointF &oPosition, bool bValid);
    void                                slotUpdateSharedMouseVPosition(const QPointF &oPosition, bool bValid);
    void                                slotUpdateSharedMouseHPosition(const QPointF &oPosition, bool bValid);

protected slots:
    virtual void                        slotUpdateScalesAndLabels();
    void                                slotSetXScaleBase(int iBase);
    void                                slotGrabFrame();
    virtual void                        slotScaleDivChanged();
    void                                slotMousePositionChanged(const QPointF &oPosition);
    void                                slotMousePositionValid(bool bValid);
    void                                slotPlotUndocked(bool bUndocked);

signals:
    void                                sigUpdateScalesAndLabels();
    void                                sigSetXScaleBase(int iBase);
    void                                sigStrobeAutoscale(unsigned int u32Delay_ms);
    void                                sigXScaleDivChanged(double dMin, double dMax);
    void                                sigSharedMousePositionChanged(const QPointF &oPosition, bool bValid);


};

#endif // QWT_PLOT_WIDGET_BASE_H
