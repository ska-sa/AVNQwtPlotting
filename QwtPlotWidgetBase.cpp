//System includes
#include <cmath>
#include <iostream>

//Library includes
#include <QPen>
#include <QPrinter>
#if QWT_VERSION < 0x060100 //For implementation of plot exporting in old versions of Qwt
#include <QFileDialog>
#include <QImageWriter>
#endif
#include <QDebug>
#include <qwt_scale_engine.h>
#include <qwt_plot_renderer.h>
#include <qwt_text_label.h>
#include <qwt_scale_widget.h>

//Local includes
#include "ui_QwtPlotWidgetBase.h"
#include "AVNUtilLibs/Timestamp/Timestamp.h"
#include "QwtPlotWidgetBase.h"

using namespace std;

cQwtPlotWidgetBase::cQwtPlotWidgetBase(QWidget *pParent) :
    QMainWindow(pParent),
    m_pUI(new Ui::cQwtPlotWidgetBase),
    m_bIsPaused(false),
    m_bIsAutoscaleEnabled(false),
    m_bTimestampInTitleEnabled(true),
    m_bDoLogConversion(false),
    m_bDoPowerLogConversion(false),
    m_bRejectData(true),
    m_bMousePositionValid(false),
    m_bVSharedMousePositionValid(false),
    m_bHSharedMousePositionValid(false)
{
    m_pUI->setupUi(this);

    //DockWidgets must exist in a MainWindow which is why this class derives MainWindow.
    //This widget can then still be put into a parent MainWindow (the actual MainWindow
    //of the application) and be used like a normal Widget.

    //All items for the plotting exist in the DockWidget which, in turn, is not in the parent MainWindow's central widget
    //QMainWindow must however always have a central widget which comes up as an empty block. Therefore, hide it.
    m_pUI->centralwidget->setVisible(false);

    //Make the axis and title font a little bit smaller
    m_oXFont = m_pUI->qwtPlot->axisTitle(QwtPlot::xBottom).font();
    m_oXFont.setPointSizeF(m_oXFont.pointSizeF() * 0.65);

    m_oYFont = m_pUI->qwtPlot->axisTitle(QwtPlot::yLeft).font();
    m_oYFont.setPointSizeF(m_oYFont.pointSizeF() * 0.65);

    m_oTitleFont = m_pUI->qwtPlot->titleLabel()->font();
    m_oTitleFont.setPointSizeF(m_oTitleFont.pointSizeF() * 0.6);

    //Set up other plot controls
    m_pPlotPositionPicker = new cQwtPlotPositionPicker(QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::NoRubberBand, QwtPicker::AlwaysOn, m_pUI->qwtPlot->canvas());
    m_pPlotPositionPicker->setTrackerPen(QPen(Qt::white));

    m_pPlotDistancePicker = new cQwtPlotDistancePicker(QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::RectRubberBand, QwtPicker::ActiveOnly, m_pUI->qwtPlot->canvas());
    m_pPlotDistancePicker->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::ControlModifier);
    m_pPlotDistancePicker->setTrackerPen(QPen(Qt::white));

    //Shared mouse position;
    m_pVSharedMousePosition = new QwtPlotMarker;
    m_pVSharedMousePosition->setLinePen( QColor(Qt::magenta) );
    m_pVSharedMousePosition->setLineStyle(QwtPlotMarker::HLine);
    m_pVSharedMousePosition->setLabelOrientation(Qt::Vertical);
    m_pVSharedMousePosition->setLabelAlignment(Qt::AlignLeft);
    m_pHSharedMousePosition = new QwtPlotMarker;
    m_pHSharedMousePosition->setLinePen( QColor(Qt::magenta) );
    m_pHSharedMousePosition->setLineStyle(QwtPlotMarker::VLine);
    m_pHSharedMousePosition->setLabelOrientation(Qt::Horizontal);
    m_pHSharedMousePosition->setLabelAlignment(Qt::AlignTop);

    m_pUI->qwtPlot->setAutoReplot(true);
    m_pUI->qwtPlot->enableAxis(QwtPlot::yRight, true);

    m_pUI->qwtPlot->axisScaleDraw(QwtPlot::yRight)->setMinimumExtent(40);
    m_pUI->qwtPlot->axisScaleDraw(QwtPlot::yLeft)->setMinimumExtent(60);

    //Set tick font to be a bit smaller than standard
    QFont oAxisFont = m_pUI->qwtPlot->axisFont(QwtPlot::yLeft);
    oAxisFont.setPointSizeF(oAxisFont.pointSizeF() * 0.8);
    m_pUI->qwtPlot->setAxisFont(QwtPlot::yLeft, oAxisFont);
    m_pUI->qwtPlot->setAxisFont(QwtPlot::yRight, oAxisFont);
    m_pUI->qwtPlot->setAxisFont(QwtPlot::xBottom, oAxisFont);
    m_pUI->qwtPlot->setAxisFont(QwtPlot::xTop, oAxisFont);

    QObject::connect(m_pUI->pushButton_pauseResume, SIGNAL(clicked()), this, SLOT(slotPauseResume()));
    QObject::connect(m_pUI->checkBox_autoscale, SIGNAL(clicked(bool)), this, SLOT(slotEnableAutoscale(bool)));
    QObject::connect(m_pUI->pushButton_grabFrame, SIGNAL(clicked()), this, SLOT(slotGrabFrame()) );

    //Keeping track of current mouse position and whether it is valid. I.e. cursor on plot canvas
    QObject::connect(m_pPlotPositionPicker, SIGNAL(moved(QPointF)), this, SLOT(slotMousePositionChanged(QPointF)) );
    QObject::connect(m_pPlotPositionPicker, SIGNAL(activated(bool)), this, SLOT(slotMousePositionValid(bool)) );

    //Connections to update plot data as well as labels and scales are forced to be queued as the actual drawing of the widget needs to be done in the GUI thread
    //This allows an update request to come from an arbirary thread to get executed by the GUI thread
    QObject::connect(this, SIGNAL(sigUpdateScalesAndLabels()), this, SLOT(slotUpdateScalesAndLabels()), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sigSetXScaleBase(int)), this, SLOT(slotSetXScaleBase(int)), Qt::QueuedConnection);

    QObject::connect(this, SIGNAL(sigStrobeAutoscale(unsigned int)), this, SLOT(slotStrobeAutoscale(unsigned int)), Qt::QueuedConnection);

    QObject::connect(m_pUI->qwtPlot->axisWidget(QwtPlot::xBottom), SIGNAL(scaleDivChanged()), this, SLOT(slotScaleDivChanged()) );

    QObject::connect(m_pUI->dockWidget, SIGNAL(topLevelChanged(bool)), this, SLOT(slotPlotUndocked(bool) ) );
}

cQwtPlotWidgetBase::~cQwtPlotWidgetBase()
{
    //If shared mouse position markers are valid they are attached to plot and will be cleaned up
    //If not, they need to be deleted
    if(!m_bHSharedMousePositionValid)
        delete m_pHSharedMousePosition;

    if(!m_bVSharedMousePositionValid)
        delete m_pVSharedMousePosition;

    delete m_pUI;
}

void cQwtPlotWidgetBase::insertWidgetIntoControlFrame(QWidget* pNewWidget, uint32_t u32Index, bool bAddSpacerAfter)
{
    QHBoxLayout* pLayout = qobject_cast<QHBoxLayout*>(m_pUI->frame_controls->layout());

    //Add the new widget
    pLayout->insertWidget(u32Index, pNewWidget);

    if(bAddSpacerAfter)
        pLayout->insertSpacerItem(u32Index + 1, new QSpacerItem(20, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));
}

void cQwtPlotWidgetBase::setXLabel(const QString &qstrXLabel)
{
    m_qstrXLabel = qstrXLabel;

    sigUpdateScalesAndLabels();
}

void cQwtPlotWidgetBase::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;

    sigUpdateScalesAndLabels();
}

void cQwtPlotWidgetBase::setXScaleIsTime(bool bXIsTime)
{
    m_pPlotPositionPicker->setXIsTime(bXIsTime);
    m_pPlotDistancePicker->setXIsTime(bXIsTime);
}

void cQwtPlotWidgetBase::setYLabel(const QString &qstrYLabel)
{
    m_qstrYLabel = qstrYLabel;

    sigUpdateScalesAndLabels();
}

void cQwtPlotWidgetBase::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;

    sigUpdateScalesAndLabels();
}

void cQwtPlotWidgetBase::setYScaleIsTime(bool bYIsTime)
{
    m_pPlotPositionPicker->setYIsTime(bYIsTime);
    m_pPlotDistancePicker->setYIsTime(bYIsTime);
}

void cQwtPlotWidgetBase::setTitle(const QString &qstrTitle)
{
    m_qstrTitle = qstrTitle;

    sigUpdateScalesAndLabels();
}

void cQwtPlotWidgetBase::showAutoscaleControl(bool bEnable)
{
    m_pUI->checkBox_autoscale->setVisible(bEnable);
}

void cQwtPlotWidgetBase::showPauseControl(bool bEnable)
{

    m_pUI->pushButton_pauseResume->setVisible(bEnable);
}

void cQwtPlotWidgetBase::slotPauseResume()
{
    slotPause(!m_bIsPaused);
}

void cQwtPlotWidgetBase::slotPause(bool bPause)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bIsPaused = bPause;

    if(m_bIsPaused)
        m_pUI->pushButton_pauseResume->setText(QString("Resume"));
    else
        m_pUI->pushButton_pauseResume->setText(QString("Pause"));
}

void cQwtPlotWidgetBase::slotEnableAutoscale()
{
    slotEnableAutoscale(true);
}

void cQwtPlotWidgetBase::slotDisableAutoscale()
{
    slotEnableAutoscale(false);
}

void cQwtPlotWidgetBase::enableTimestampInTitle(bool bEnable)
{
    m_bTimestampInTitleEnabled = bEnable;

    if(!m_bTimestampInTitleEnabled)
        m_pUI->qwtPlot->setTitle(m_qstrTitle);
}

void cQwtPlotWidgetBase::enableLogConversion(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bDoLogConversion = bEnable;

    if(m_bDoLogConversion)
        m_bDoPowerLogConversion = false;
}

void cQwtPlotWidgetBase::enablePowerLogConversion(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bDoPowerLogConversion = bEnable;

    if(m_bDoPowerLogConversion)
        m_bDoLogConversion = false;
}

void cQwtPlotWidgetBase::enableRejectData(bool bEnable)
{
    QWriteLocker oWriteLock(&m_oMutex);

    m_bRejectData = bEnable;
}

void cQwtPlotWidgetBase::slotGrabFrame()
{
    slotPause(true);

    QwtPlotRenderer oRenderer;

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions

    //Implement Qwt's new exportTo function (copied and adapted from Qwt source):

    if ( m_pUI->qwtPlot == NULL )
        return;

    QString qstrFileName = QString("%1").arg(m_pUI->qwtPlot->title().text());

    // What about translation

#ifndef QT_NO_FILEDIALOG
    const QList<QByteArray> qlImageFormats = QImageWriter::supportedImageFormats();

    QStringList oFilter;
#ifndef QT_NO_PRINTER
    oFilter += QString( "PDF " ) + tr( "Documents" ) + " (*.pdf)";
#endif
#ifndef QWT_NO_SVG
    oFilter += QString( "SVG " ) + tr( "Documents" ) + " (*.svg)";
#endif
#ifndef QT_NO_PRINTER
    oFilter += QString( "Postscript " ) + tr( "Documents" ) + " (*.ps)";
#endif

    if ( qlImageFormats.size() > 0 )
    {
        QString qstrImageFilter( tr( "Images" ) );
        qstrImageFilter += " (";
        for ( int i = 0; i < qlImageFormats.size(); i++ )
        {
            if ( i > 0 )
                qstrImageFilter += " ";
            qstrImageFilter += "*.";
            qstrImageFilter += qlImageFormats[i];
        }
        qstrImageFilter += ")";

        oFilter += qstrImageFilter;
    }

    qstrFileName = QFileDialog::getSaveFileName(NULL, tr( "Export File Name" ), qstrFileName, oFilter.join( ";;" ), NULL, QFileDialog::DontConfirmOverwrite );
#endif
    if ( qstrFileName.isEmpty() )
        return;

    QSizeF oSize(297.0, 210.0);
    oRenderer.renderDocument( m_pUI->qwtPlot, qstrFileName, oSize, 300 );

#else
    oRenderer.exportTo(m_pUI->qwtPlot, QString("%1").arg(m_pUI->qwtPlot->title().text()), QSizeF(297.0, 210.0), 300);
#endif


    slotPause(false);
}

void cQwtPlotWidgetBase::slotSetXScaleBase(int iBase)
{
#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    Q_UNUSED(iBase);
    //Setting base not supported in this version of Qtwt

#else
    m_pUI->qwtPlot->axisScaleEngine(QwtPlot::xBottom)->setBase(iBase);
#endif
}

void cQwtPlotWidgetBase::strobeAutoscale(uint32_t u32Delay_ms)
{
    //Signal with queued connection to slot for thread decoupling
    sigStrobeAutoscale(u32Delay_ms);
}

void cQwtPlotWidgetBase::slotStrobeAutoscale(unsigned int u32Delay_ms)
{
    cout << "cQwtPlotWidgetBase::strobeAutoscale() strobing autoscale for plot " << m_qstrTitle.toStdString() << " in " << u32Delay_ms << " ms." << endl;

    //If autoscale is already on for this plot do nothing
    if(m_bIsAutoscaleEnabled)
        return;

    QTimer::singleShot(u32Delay_ms, this, SLOT(slotEnableAutoscale()));
    QTimer::singleShot(u32Delay_ms + 100, this, SLOT(slotDisableAutoscale())); //Switch off 100 ms later
}


void cQwtPlotWidgetBase::slotUpdateScalesAndLabels()
{
    if(m_qstrXUnit.length())
    {
        QwtText oXLabel(QwtText(QString("%1 [%2]").arg(m_qstrXLabel).arg(m_qstrXUnit)) );
        oXLabel.setFont(m_oXFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, oXLabel);
    }
    else
    {
        QwtText oXLabel( QwtText(QString("%1").arg(m_qstrXLabel)) );
        oXLabel.setFont(m_oXFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::xBottom, oXLabel);
    }

    if(m_qstrYUnit.length())
    {
        QwtText oYLabel( QString("%1\n[%2]").arg(m_qstrYLabel).arg(m_qstrYUnit) );
        oYLabel.setFont(m_oYFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, oYLabel);
    }
    else
    {
        QwtText oYLabel( QString("%1\n").arg(m_qstrYLabel) );
        oYLabel.setFont(m_oYFont);

        m_pUI->qwtPlot->setAxisTitle(QwtPlot::yLeft, oYLabel);
    }

    QwtText oYLabel( QString(" "));
    oYLabel.setFont(m_oYFont);

    m_pUI->qwtPlot->setAxisTitle(QwtPlot::yRight, oYLabel);

    m_pPlotPositionPicker->setXUnit(m_qstrXUnit);
    m_pPlotPositionPicker->setYUnit(m_qstrYUnit);

    m_pPlotDistancePicker->setXUnit(m_qstrXUnit);
    m_pPlotDistancePicker->setYUnit(m_qstrYUnit);

    QwtText oTitle(m_qstrTitle);
    oTitle.setFont(m_oTitleFont);
    m_pUI->qwtPlot->setTitle(oTitle);
}

void cQwtPlotWidgetBase::autoUpdateXScaleBase(uint32_t u32NBins)
{
    //Set the X scale to base 2 ticks if the number of bins is a power of 2

    //Check if number of points to plot is base 2:
    double dExponent = log2(u32NBins);
    double dIntegerPart; //Unused

    if(modf(dExponent, &dIntegerPart) == 0.0)
    {
        sigSetXScaleBase(2);
        //Plot grid lines along base 2 numbers
    }
    else
    {
        sigSetXScaleBase(10);
        //Otherwise plot grid lines along base 10 numbers
    }
}

void cQwtPlotWidgetBase::slotUpdateXScaleDiv(double dMin, double dMax)
{
    m_pUI->qwtPlot->setAxisScale(QwtPlot::xBottom, dMin, dMax);
    m_pUI->qwtPlot->replot();
}

void cQwtPlotWidgetBase::slotScaleDivChanged()
{
    sigXScaleDivChanged(m_pUI->qwtPlot->axisInterval(QwtPlot::xBottom).minValue(), m_pUI->qwtPlot->axisInterval(QwtPlot::xBottom).maxValue());
}

void cQwtPlotWidgetBase::slotMousePositionValid(bool bValid)
{
    m_bMousePositionValid = bValid;

    //Explicitly notify when the mouse position is not plot valid.
    //I.e. mouse cursor is no longer on the plot
    if(!m_bMousePositionValid)
        sigSharedMousePositionChanged(QPointF(0.0, 0.0), false);
}

void cQwtPlotWidgetBase::slotMousePositionChanged(const QPointF &oPosition)
{
    //Relay for access outside of this class
    sigSharedMousePositionChanged(oPosition, m_bMousePositionValid);
}

void cQwtPlotWidgetBase::slotUpdateSharedMousePosition(const QPointF &oPosition, bool bValid)
{
    slotUpdateSharedMouseVPosition(oPosition, bValid);
    slotUpdateSharedMouseHPosition(oPosition, bValid);
}

void cQwtPlotWidgetBase::slotUpdateSharedMouseVPosition(const QPointF &oPosition, bool bValid)
{
    if(m_bVSharedMousePositionValid && !bValid)
    {
        m_pVSharedMousePosition->detach();
        m_bVSharedMousePositionValid = bValid;
        return;
    }

    if(!m_bVSharedMousePositionValid && bValid)
    {
        m_pVSharedMousePosition->attach(m_pUI->qwtPlot);
        m_bVSharedMousePositionValid = bValid;
    }

    m_pVSharedMousePosition->setYValue(oPosition.y());
}

void cQwtPlotWidgetBase::slotUpdateSharedMouseHPosition(const QPointF &oPosition, bool bValid)
{
    if(m_bHSharedMousePositionValid && !bValid)
    {
        m_pHSharedMousePosition->detach();
        m_bHSharedMousePositionValid = bValid;
        return;
    }

    if(!m_bHSharedMousePositionValid && bValid)
    {
        m_pHSharedMousePosition->attach(m_pUI->qwtPlot);
        m_bHSharedMousePositionValid = bValid;
    }

    m_pHSharedMousePosition->setXValue(oPosition.x());
}

void cQwtPlotWidgetBase::slotPlotUndocked(bool bUndocked)
{
    cout << "cQwtPlotWidgetBase::slotPlotUndocked(): Got undocked = " << bUndocked << " for plot " << m_qstrTitle.toStdString() << endl;

    //If the dockWidget is undocked, hide the parent window to relieve space in main GUI
    if(bUndocked)
    {
        setVisible(false);
    }
    else
    {
        setVisible(true);
    }

}
