//Extension of the standard QwtPlotZoomer that progressively expands the
//QwtPlotCanvas progressively in an animation.
//Essentially non-essential eye-candy :P

#ifndef ANIMATED_QWT_PLOT_ZOOMER_H
#define ANIMATED_QWT_PLOT_ZOOMER_H

//System includes
#ifdef _WIN32
#include <stdint.h>
#else
#include <inttypes.h>
#endif

//Library includes
#include <QTimer>
#include <qwt_plot_zoomer.h>
#include <QReadWriteLock>

//Local includes

class cAnimatedQwtPlotZoomer : public QwtPlotZoomer
{
    Q_OBJECT
public:

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    explicit cAnimatedQwtPlotZoomer(QwtPlotCanvas *pCanvas, bool bDoReplot = true, bool bEnableAnimation = true, uint32_t u32AnimationDuration_ms = 800);
    explicit cAnimatedQwtPlotZoomer(int iXAxis, int iYAxis, QwtPlotCanvas *pCanvas, bool bDoReplot = true, bool bEnableAnimation = true, uint32_t u32AnimationDuration_ms = 800);
#else
    explicit cAnimatedQwtPlotZoomer(QWidget *pCanvas, bool bDoReplot = true, bool bEnableAnimation = true, uint32_t u32AnimationDuration_ms = 800);
    explicit cAnimatedQwtPlotZoomer(int iXAxis, int iYAxis, QWidget *pCanvas, bool bDoReplot = true, bool bEnableAnimation = true, uint32_t u32AnimationDuration_ms = 800);
#endif

    void                    setAnimationDuration(uint32_t u32AnimationDuration_ms);

    void                    setAnimationEnabled(bool bEnableAnimation);
    bool                    isAnimationEnabled();

    bool                    isCurrentlyAnimating();

protected:
    const static uint32_t   FPS = 30;
    uint32_t                m_u32AnimationDuration_ms;
    uint32_t                m_u32NAnimationFrames;
    uint32_t                m_u32AnimationFrameCount;

    virtual void            rescale();
    virtual void            setAxisScales(double dX1, double dX2, double dY1, double dY2);
    virtual bool            end( bool bOK = true );
    virtual void            widgetMouseReleaseEvent( QMouseEvent *pEvent );

    QTimer                  m_oFrameTimer;

    double                  m_dZoomTargetX1;
    double                  m_dZoomTargetX2;
    double                  m_dZoomTargetY1;
    double                  m_dZoomTargetY2;

    QwtPlot                 *m_pPlot;

    bool                    m_bAnimate;
    bool                    m_bAnimationEnabled;

    bool                    m_bCurrenlyAnimating;

    QReadWriteLock          m_oMutex;

signals:

private slots:
    void                    slotGenerateAnimationFrame();

public slots:

};

#endif // ANIMATED_QWT_PLOT_ZOOMER_H
