#ifndef WATERFALL_PLOT_SPECTROMGRAM_DATA_H
#define WATERFALL_PLOT_SPECTROMGRAM_DATA_H

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
#include <qwt_raster_data.h>

//Local includes

class cWaterfallPlotSpectromgramData : public QwtRasterData
{
public:
    cWaterfallPlotSpectromgramData();

    virtual double              value(double dX, double dY ) const;
    virtual void                setInterval(Qt::Axis eAxis, const QwtInterval & oInterval);

    void                        addFrame(const QVector<float> &qvfNewFrame, int64_t i64Timestamp_us);
    void                        setDimensions(uint32_t u32X, uint32_t u32Y);

    int64_t                     getMinTime_us() const;
    int64_t                     getMaxTime_us() const;

    void                        getZMinMaxValue(double &dZMin, double &dZMax) const;
    double                      getMedian() const;

    void                        enableLogConversion(bool bEnable);
    void                        enablePowerLogConversion(bool bEnable);

private:
    QVector< QVector<float> >   m_qvvfCircularBuffer;
    QVector<int64_t>            m_qvi64Timestamps;

    uint32_t                    m_u32NextFrameIndex;

    uint32_t                    m_u32NRows;
    uint32_t                    m_u32NColumns;

    double                      m_dDeltaX;
    double                      m_dDeltaY;

    bool                        m_bDoLogConversion;
    bool                        m_bDoPowerLogConversion;

    void                        update();

    uint32_t                    unwrapCircularBufferIndex(uint32_t u32LinearIndex) const;
};

#endif // CWATERFALLPLOTSPECTROMGRAMDATA_H
