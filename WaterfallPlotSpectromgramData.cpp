//System includes
#include <iostream>
#include <cmath>
#include <cfloat>

//Library includes

//Local includes
#include "WaterfallPlotSpectromgramData.h"
#include "AVNUtilLibs/Timestamp/Timestamp.h"

using namespace std;

cWaterfallPlotSpectromgramData::cWaterfallPlotSpectromgramData() :
    m_u32NextFrameIndex(0),
    m_u32NRows(0),
    m_u32NColumns(0)
{
}

double cWaterfallPlotSpectromgramData::value( double dX, double dY ) const
{
    //Adapted from qwt_matrix_raster_data.cpp

    //cout << "dY = " << AVN::stringFromTimestamp_HHmmss(dY * 1e6) << endl;

    const QwtInterval oXInterval = interval( Qt::XAxis );
    const QwtInterval oYInterval = interval( Qt::YAxis );

    //cout << "interval = " << (dY - oYInterval.minValue()) / m_dDeltaY << endl;

    uint32_t ui32Row = uint32_t( (dY - oYInterval.minValue() ) / m_dDeltaY );
    uint32_t ui32Col = uint32_t( (dX - oXInterval.minValue() ) / m_dDeltaX );

    // In case of intervals, where the maximum is included
    // we get out of bound for row/col, when the value for the
    // maximum is requested. Instead we return the value
    // from the last row/col

    if ( ui32Row >= m_u32NRows )
        ui32Row = m_u32NRows - 1;

    if ( ui32Col >= m_u32NColumns )
        ui32Col = m_u32NColumns - 1;

    return 10.0 * log10(m_qvvfCircularBuffer[unwrapCircularBufferIndex(ui32Row)][ui32Col] + 0.001);
}

void cWaterfallPlotSpectromgramData::setInterval( Qt::Axis eAxis, const QwtInterval &oInterval)
{
    QwtRasterData::setInterval( eAxis, oInterval );
    update();
}

void cWaterfallPlotSpectromgramData::addFrame(const QVector<float> &qvfNewFrame, int64_t i64Timestamp_us)
{
    //Check that the spectrogram is the right width. Update as necessary
    if(qvfNewFrame.size() != m_qvvfCircularBuffer[0].size())
    {
        setDimensions(qvfNewFrame.size(), m_qvvfCircularBuffer.size());
    }

    //Copy the data into the next index
    std::copy(qvfNewFrame.begin(), qvfNewFrame.end(), m_qvvfCircularBuffer[m_u32NextFrameIndex].begin());

    //Copy the timestamp into the corresponding index
    m_qvi64Timestamps[m_u32NextFrameIndex] = i64Timestamp_us;

    //Increment the next
    m_u32NextFrameIndex++;

    //Unwrap the next index as necessary
    if(m_u32NextFrameIndex >= (uint32_t)m_qvvfCircularBuffer.size())
    {
        m_u32NextFrameIndex = 0;
    }

    setInterval(Qt::YAxis, QwtInterval(getMinTime_us() / 1e6, getMaxTime_us() / 1e6));
}

void cWaterfallPlotSpectromgramData::setDimensions(uint32_t u32X, uint32_t u32Y)
{
    m_qvvfCircularBuffer.resize(u32Y);

    for(uint32_t ui = 0; ui < (uint32_t)m_qvvfCircularBuffer.size(); ui++)
    {
        m_qvvfCircularBuffer[ui].resize(u32X);
    }

    m_qvi64Timestamps.resize(u32Y);

    m_u32NColumns = u32X;
    m_u32NRows = u32Y;
}

void cWaterfallPlotSpectromgramData::update()
{
    if(!m_qvvfCircularBuffer.size())
        return;

    const QwtInterval oXInterval = interval( Qt::XAxis );
    const QwtInterval oYInterval = interval( Qt::YAxis );

    if ( oXInterval.isValid() )
        m_dDeltaX = oXInterval.width() / m_u32NColumns;
    if ( oYInterval.isValid() )
        m_dDeltaY = oYInterval.width() / m_u32NRows;
}

int64_t cWaterfallPlotSpectromgramData::getMaxTime_us() const
{
    int32_t i32Index = m_u32NextFrameIndex - 1;

    if(i32Index == -1)
        i32Index = m_u32NRows - 1;

    return m_qvi64Timestamps[i32Index];
}

int64_t cWaterfallPlotSpectromgramData::getMinTime_us() const
{
    return m_qvi64Timestamps[m_u32NextFrameIndex];
}

uint32_t cWaterfallPlotSpectromgramData::unwrapCircularBufferIndex(uint32_t u32LinearIndex) const
{
    //Unwrap circular buffer:
    int32_t i32ActualIndex = (int32_t)u32LinearIndex + (int32_t)m_u32NextFrameIndex - 1;

    if(i32ActualIndex >= (int32_t)m_u32NRows)
        i32ActualIndex -= m_u32NRows;

    if(i32ActualIndex < 0 )
        i32ActualIndex += m_u32NRows;

    return i32ActualIndex;
}

void cWaterfallPlotSpectromgramData::getZMinMaxValue(double &dZMin, double &dZMax) const
{
    double dZMaxTmp = DBL_MIN;
    double dZMinTmp = DBL_MAX;

    for(uint32_t u32Y = 0; u32Y < (uint32_t)m_qvvfCircularBuffer.size(); u32Y++)
    {
        for(uint32_t u32X = 0; u32X < (uint32_t)m_qvvfCircularBuffer[u32Y].size(); u32X++)
        {
            if(m_qvvfCircularBuffer[u32Y][u32X] > dZMaxTmp)
                dZMaxTmp = m_qvvfCircularBuffer[u32Y][u32X];

            if(m_qvvfCircularBuffer[u32Y][u32X] < dZMinTmp)
                dZMinTmp = m_qvvfCircularBuffer[u32Y][u32X];
        }
    }

    dZMax = dZMaxTmp;
    dZMin = dZMinTmp;
}

