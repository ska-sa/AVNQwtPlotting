
//System includes
#include <iostream>

//Library includes
#include <qwt_plot.h>
#include <qwt_scale_div.h>
#include <QMouseEvent>

//Local includes
#include "AnimatedQwtPlotZoomer.h"

using namespace std;

cAnimatedQwtPlotZoomer::cAnimatedQwtPlotZoomer( QWidget *pCanvas, bool bDoReplot, bool bEnableAnimation, uint32_t u32AnimationDuration_ms) :
    QwtPlotZoomer(pCanvas, bDoReplot),
    m_u32NAnimationFrames(1),
    m_u32AnimationFrameCount(0),
    m_dZoomTargetX1(0.0),
    m_dZoomTargetX2(0.0),
    m_dZoomTargetY1(0.0),
    m_dZoomTargetY2(0.0),
    m_pPlot(plot()),
    m_bAnimate(false),
    m_bAnimationEnabled(bEnableAnimation)
{
    setAnimationDuration(u32AnimationDuration_ms);

    QObject::connect( &m_oFrameTimer, SIGNAL(timeout()), this, SLOT(slotGenerateAnimationFrame()) );
}

cAnimatedQwtPlotZoomer::cAnimatedQwtPlotZoomer(int iXAxis, int iYAxis, QWidget *pCanvas, bool bDoReplot, bool bEnableAnimation, uint32_t u32AnimationDuration_ms) :
    QwtPlotZoomer(iXAxis, iYAxis, pCanvas, bDoReplot),
    m_u32NAnimationFrames(1),
    m_u32AnimationFrameCount(0),
    m_dZoomTargetX1(0.0),
    m_dZoomTargetX2(0.0),
    m_dZoomTargetY1(0.0),
    m_dZoomTargetY2(0.0),
    m_pPlot(plot()),
    m_bAnimate(false),
    m_bAnimationEnabled(bEnableAnimation)
{
    setAnimationDuration(u32AnimationDuration_ms);

    QObject::connect( &m_oFrameTimer, SIGNAL(timeout()), this, SLOT(slotGenerateAnimationFrame()) );
}

void cAnimatedQwtPlotZoomer::setAnimationDuration(uint32_t u32AnimationDuration_ms)
{
    m_u32AnimationDuration_ms = u32AnimationDuration_ms;
    m_u32NAnimationFrames = u32AnimationDuration_ms / (1000.0 / FPS);

    if(m_u32NAnimationFrames < 1.0 || u32AnimationDuration_ms == 0)
        m_u32NAnimationFrames = 1;
}

void cAnimatedQwtPlotZoomer::setAnimationEnabled(bool bAnimationEnabled)
{
    m_bAnimationEnabled = bAnimationEnabled;
}

bool cAnimatedQwtPlotZoomer::isAnimationEnabled()
{
    return m_bAnimationEnabled;
}

void cAnimatedQwtPlotZoomer::cAnimatedQwtPlotZoomer::rescale()
{
    //Largely derived from orginal Qwt code
    //The new scale is just progressively drawn
    if ( !m_pPlot )
        return;

    const QRectF &rect = zoomStack()[zoomRectIndex()];
    if ( rect != scaleRect() )
    {
        m_dZoomTargetX1 = rect.left();
        m_dZoomTargetX2 = rect.right();
        if ( !m_pPlot->axisScaleDiv( xAxis() ).isIncreasing() )
            qSwap( m_dZoomTargetX1, m_dZoomTargetX2 );

        m_dZoomTargetY1 = rect.top();
        m_dZoomTargetY2 = rect.bottom();
        if ( !m_pPlot->axisScaleDiv( yAxis() ).isIncreasing() )
            qSwap( m_dZoomTargetY1, m_dZoomTargetY2 );
    }

    if(m_bAnimate && m_bAnimationEnabled)
    {
        {
            QWriteLocker oLock(&m_oMutex);
            m_bCurrenlyAnimating = true;
        }

        //Generate a sequence of zoom frames with a timer
        m_u32AnimationFrameCount = 0;
        m_oFrameTimer.start(1000.0 / FPS);
    }
    else
    {
        //Go directly to the desired zoom
        setAxisScales(m_dZoomTargetX1, m_dZoomTargetX2, m_dZoomTargetY1, m_dZoomTargetY2);
    }
}

void cAnimatedQwtPlotZoomer::setAxisScales(double dX1, double dX2, double dY1, double dY2)
{
    if ( !m_pPlot )
        return;

    //cout << " dX1 = " << dX1 << ", dX2 = " << dX2 << ", dY1 = " << dY1 << ", dY2 = " << dY2 << endl;

    const bool doReplot = m_pPlot->autoReplot();
    m_pPlot->setAutoReplot( false );

    m_pPlot->setAxisScale( xAxis(), dX1, dX2 );
    m_pPlot->setAxisScale( yAxis(), dY1, dY2 );

    m_pPlot->setAutoReplot( doReplot );

    m_pPlot->replot();
}

void cAnimatedQwtPlotZoomer::slotGenerateAnimationFrame()
{

    //cout << m_u32AnimationFrameCount << " of " << m_u32NAnimationFrames << endl;

    if(m_u32AnimationFrameCount >= m_u32NAnimationFrames)
    {
        //Stop the timer and go to the final zoom target
        m_oFrameTimer.stop();
        setAxisScales(m_dZoomTargetX1, m_dZoomTargetX2, m_dZoomTargetY1, m_dZoomTargetY2);

        QWriteLocker oLock(&m_oMutex);
        m_bCurrenlyAnimating = false;
    }
    else
    {
        //Go a fraction of the way to the zoom target from the current position. This results in logarithmic deaceleration
        //Currently 20% of this distance seems to give a good smooth transistion with minimal jump at the end for the default 800 ms animation
        double dX1 = m_dZoomTargetX1 - ( (m_dZoomTargetX1 - m_pPlot->axisInterval(xAxis()).minValue()) / 1.2 );
        double dX2 = m_dZoomTargetX2 - ( (m_dZoomTargetX2 - m_pPlot->axisInterval(xAxis()).maxValue()) / 1.2 );
        double dY1 = m_dZoomTargetY1 - ( (m_dZoomTargetY1 - m_pPlot->axisInterval(yAxis()).minValue()) / 1.2 );
        double dY2 = m_dZoomTargetY2 - ( (m_dZoomTargetY2 - m_pPlot->axisInterval(yAxis()).maxValue()) / 1.2 );

        setAxisScales(dX1, dX2, dY1, dY2);
    }

    m_u32AnimationFrameCount++;
}

bool cAnimatedQwtPlotZoomer::end( bool bOK )
{
    //This function is called for rubberband banded zooming.
    //Here we animate

    m_bAnimate = true;

    bool bResult = QwtPlotZoomer::end(bOK);

    m_bAnimate = false;

    return bResult;
}

void cAnimatedQwtPlotZoomer::widgetMouseReleaseEvent( QMouseEvent *pEvent)
{
    //This function calls other zooming based on the stack
    //Here we also animate if we are not using the middle button

    if(pEvent->button() != Qt::MiddleButton)
        m_bAnimate = true;

    QwtPlotZoomer::widgetMouseReleaseEvent(pEvent);

    m_bAnimate = false;
}

bool cAnimatedQwtPlotZoomer::isCurrentlyAnimating()
{
    QReadLocker oLock(&m_oMutex);

    return m_bCurrenlyAnimating;
}
