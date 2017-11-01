/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * timeUtil.hpp
 *
 *  Created on: Sep 20, 2013
 *      Author: petero
 */

#ifndef TIMEUTIL_HPP_
#define TIMEUTIL_HPP_

#include "util.hpp"

#include <array>
#include <cmath>
#include <cassert>


/** Return current wall clock time in milliseconds, starting at some arbitrary point in time. */
S64 currentTimeMillis();

/** Return current wall clock time in seconds, starting at some arbitrary point in time. */
double currentTime();



/** Class that measures average CPU utilization. */
class UtilizationTimer {
public:
    /** Constructor. All times start at zero. */
    UtilizationTimer();

    /** Reset times to zero. */
    void reset();

    /** Set the current CPU efficiency factor to p.
     * A negative p indicates the CPU is idle. */
    void setPUseful(double p);

    /** Return elapsed time since last reset, the amount of useful time spent,
     * and the amount of time the CPU has been idle. */
    void getStats(double& elapsed, double& useful, double& sleep);

private:
    /** Update tElapsed, tUseful, tSleep and increase t0 to current time. */
    void update();

    double t0;          // Time stand for last update()
    double pUseful;     // Current CPU efficiency factor

    double tElapsed;    // Total elapsed time since reset()
    double tUseful;     // Total spent useful time
    double tSleep;      // total idle time.
};

/** Class that tracks statistics for a set of samples. */
class SampleStatistics {
public:
    /** Constructor. */
    SampleStatistics();

    /** Remove all samples. */
    void reset();

    /** Add a sample. */
    void addSample(double value);

    /** Return number of samples. */
    int numSamples() const;
    /** Return average sample value. */
    double avg() const;
    /** Return standard deviation of samples. */
    double std() const;

    /** Add other to *this. */
    SampleStatistics& operator+=(const SampleStatistics& other);

private:
    int nSamples;
    double sum;
    double sqSum;
};

/** Gather statistics about time samples. */
class TimeSampleStatistics {
public:
    /** Constructor. */
    TimeSampleStatistics();

    /** Start timer. */
    void start();

    /** Stop timer and add elapsed time as a sample. */
    void stop();

    /** Remove all time samples. */
    void reset();

    /** Return true if timer is currently running. */
    bool isStarted() const;

    /** Return number of time samples. */
    int numSamples() const;
    /** Return average sample time. */
    double avg() const;
    /** Return standard deviation of time samples. */
    double std() const;

    /** Print to "os", time values displayed in nanoseconds. */
    void printNs(std::ostream& os) const;

private:
    double t0;
    bool started;
    SampleStatistics stats;
};

/** A fixed size vector of TimeSampleStatistics objects. */
template <int N>
class TimeSampleStatisticsVector {
private:
    std::array<TimeSampleStatistics, N> vec;

public:
    TimeSampleStatistics& operator[](int i);

    using iterator = typename std::array<TimeSampleStatistics, N>::iterator;
    iterator begin();
    iterator end();
};

/** Utility function to add a time sample to a TimeSampleStatistics object.
 * The sample value is equal to the amount of time this object is in scope. */
class ScopedTimeSample {
public:
    explicit ScopedTimeSample(TimeSampleStatistics& tStat);
    ~ScopedTimeSample();
    ScopedTimeSample(ScopedTimeSample&) = delete;
    ScopedTimeSample& operator=(const ScopedTimeSample&) = delete;
private:
    TimeSampleStatistics& timeStat;
};


inline
UtilizationTimer::UtilizationTimer() {
    reset();
}

inline void
UtilizationTimer::reset() {
    t0 = currentTime();
    pUseful = -1;
    tElapsed = 0;
    tUseful = 0;
    tSleep = 0;
}

inline void
UtilizationTimer::setPUseful(double p) {
    update();
    pUseful = p;
}

inline void
UtilizationTimer::getStats(double& elapsed, double& useful, double& sleep) {
    update();
    elapsed = tElapsed;
    useful = tUseful;
    sleep = tSleep;
}

inline void
UtilizationTimer::update() {
    double tNow = currentTime();
    double dt = tNow - t0;
    tElapsed += dt;
    if (pUseful >= 0)
        tUseful += dt * pUseful;
    else
        tSleep += dt;
    t0 = tNow;
}


inline
SampleStatistics::SampleStatistics() {
    reset();
}

inline void
SampleStatistics::reset() {
    nSamples = 0;
    sum = 0.0;
    sqSum = 0.0;
}

inline void
SampleStatistics::addSample(double value) {
    nSamples++;
    sum += value;
    sqSum += value * value;
}

inline int
SampleStatistics::numSamples() const {
    return nSamples;
}

inline double
SampleStatistics::avg() const {
    return nSamples > 0 ? sum / nSamples : 0;
}

inline double
SampleStatistics::std() const {
    if (nSamples < 2)
        return 0;
    return ::sqrt((sqSum - sum*sum / nSamples) / (nSamples - 1));
}

inline void
TimeSampleStatistics::printNs(std::ostream& os) const {
    os << numSamples()
       << ' ' << static_cast<int>(avg() * 1e9)
       << ' ' << static_cast<int>(std() * 1e9);
}


inline
TimeSampleStatistics::TimeSampleStatistics() {
    reset();
}

inline void
TimeSampleStatistics::start() {
    t0 = currentTime();
    started = true;
}

inline void
TimeSampleStatistics::stop() {
    double now = currentTime();
    stats.addSample(now - t0);
    started = false;
}

inline void
TimeSampleStatistics::reset() {
    stats.reset();
    started = false;
}

inline bool
TimeSampleStatistics::isStarted() const {
    return started;
}

inline int
TimeSampleStatistics::numSamples() const {
    return stats.numSamples();
}

inline double
TimeSampleStatistics::avg() const {
    return stats.avg();
}

inline double
TimeSampleStatistics::std() const {
    return stats.std();
}

template <int N>
inline TimeSampleStatistics&
TimeSampleStatisticsVector<N>::operator[](int i) {
    return vec[i];
}

template <int N>
inline typename TimeSampleStatisticsVector<N>::iterator
TimeSampleStatisticsVector<N>::begin() {
    return vec.begin();
}

template <int N>
inline typename TimeSampleStatisticsVector<N>::iterator
TimeSampleStatisticsVector<N>::end() {
    return vec.end();
}


inline
ScopedTimeSample::ScopedTimeSample(TimeSampleStatistics& tStat)
  : timeStat(tStat) {
    assert(!timeStat.isStarted());
    timeStat.start();
}

inline
ScopedTimeSample::~ScopedTimeSample() {
    assert(timeStat.isStarted());
    timeStat.stop();
}


#endif /* TIMEUTIL_HPP_ */
