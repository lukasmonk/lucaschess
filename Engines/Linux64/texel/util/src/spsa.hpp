/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * spsa.hpp
 *
 *  Created on: Feb 21, 2015
 *      Author: petero
 */

#ifndef SPSA_HPP_
#define SPSA_HPP_

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <memory>
#include <cmath>
#include "util/random.hpp"

/** Run SPSA optimization. */
class Spsa {
public:
     /** Run nSimul simulations to estimate the error when running nGames games. */
    static void gameSimulation(double meanResult, double drawProb,
                               int nGames, int nSimul);

    /** Simulate a match between two engines. */
    static void engineSimulation(int nGames, const std::vector<double>& params);

    /** Simulate a round-robin tournament. */
    static void tourneySimulation(int nSimul, int nRounds, const std::vector<double>& elo);

    /** Simulate SPSA optimization nSimul times, using nIter*gamesPerIter games each time. */
    static void spsaSimulation(int nSimul, int nIter, int gamesPerIter, double a, double c,
                               const std::vector<double>& startParams);

    /** Run SPSA optimization with parameters given by the configuration file. */
    static void spsa(const std::string& configFile);
};

/** Simulate game results given win, draw and loss probabilities. */
class ResultSimulation {
public:
    /**
     * Create a simulation object with the given parameters.
     * meanResult = winP + drawP * 0.5 + lossP * 0
     * winP + drawP + lossP = 1
     * drawP = drawProb
     */
    ResultSimulation(double meanResult, double drawProb);

    /** Change simulation parameters. */
    void setParams(double meanResult, double drawProb);

    /** Simulate a number of games and return the relative score rate, (nWin+nDraw/2)/nGames. */
    double simulate(int nGames);

    /** Simulate one game and return the result, 0, 0.5 or 1. Faster than simulate(1). */
    double simulateOneGame();

    /** Simulate a number of games and return the number of wins, draws and losses. */
    void simulate(int nGames, int& nWin, int& nDraw, int& nLoss);

    /** Get the ELO difference corresponding to a mean result. */
    static double resultToElo(double meanResult);

    /** Get the mean result corresponding to an ELO difference. */
    static double eloToResult(double elo);

private:
    // Win, draw, loss probabilities
    double winP, drawP, lossP;

    std::shared_ptr<gsl_rng> rng;
    Random rnd;
};

class EnginePair {
    /** Simulate nGames games and return the average score. */
    virtual double simulate(int nGames) = 0;
};

class SimulatedEnginePair : public EnginePair {
public:
    /** Constructor. */
    SimulatedEnginePair();

    void setParams(const std::vector<double>& params1,
                   const std::vector<double>& params2);

    double simulate(int nGames) override;

    /** Get elo for given set of parameters. */
    static double getElo(const std::vector<double>& params);

private:
    ResultSimulation rs;
};


inline double
ResultSimulation::resultToElo(double meanResult) {
    return -400 * log10(1/meanResult -1);
}

inline double
ResultSimulation::eloToResult(double elo) {
    return 1 / (1 + pow(10, -elo/400));
}

#endif /* SPSA_HPP_ */
