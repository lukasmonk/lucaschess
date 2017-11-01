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
 * spsa.cpp
 *
 *  Created on: Feb 21, 2015
 *      Author: petero
 */

#include "spsa.hpp"
#include "util/timeUtil.hpp"
#include "parameters.hpp"
#include "chesstool.hpp"
#include "chessParseError.hpp"
#include <deque>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <limits>


template<class T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& v)
{
    s << "[";
    for (typename std::vector<T>::const_iterator i = v.begin(); i != v.end(); ++i) {
        if (i != v.begin())
            s << ",";
        s << " " << (*i);
    }
    s << " ]";
    return s;
}


static Random seeder;

void
Spsa::gameSimulation(double meanResult, double drawProb,
                     int nGames, int nSimul) {
    ResultSimulation rs(meanResult, drawProb);
    double sum = 0;
    double sum2 = 0;
    for (int i = 0; i < nSimul; i++) {
        double score = rs.simulate(nGames);
        sum += score;
        sum2 += score * score;
//        std::cout << "score:" << score << std::endl;
    }
    double mean = sum / nSimul;
    double N = nSimul;
    double var = (sum2 - sum * sum / N) / (N - 1);
    double s = sqrt(var);
    std::cout << "mean:" << mean << " std:" << s << " meanElo: " << ResultSimulation::resultToElo(mean) << std::endl;
    for (int si = 1; si <= 5; si++) {
        std::cout << "i:" << si << ' '
                  << std::setw(10) << ResultSimulation::resultToElo(mean - s * si)
                  << std::setw(10) << ResultSimulation::resultToElo(mean + s * si)
                  << std::endl;
    }
}

void
Spsa::engineSimulation(int nGames, const std::vector<double>& params) {
    std::vector<double> params1, params2;
    params1.assign(params.begin(), params.begin() + (params.size() / 2));
    params2.assign(params.begin() + (params.size() / 2), params.end());
    SimulatedEnginePair se;
    se.setParams(params1, params2);
    double result = se.simulate(nGames);
    std::cout << "p0: " << params[0] << " p1: " << params[1]
              << "p2: " << params[2] << " p3: " << params[3]
              << " result: " << result << " elo: " << ResultSimulation::resultToElo(result)
              << std::endl;
}

void
Spsa::tourneySimulation(int nSimul, int nRounds, const std::vector<double>& elo) {
    const int N = elo.size();
//    std::cout << "N:" << N << std::endl;
    assert(N >= 2);
    assert(nSimul > 0);
    std::vector<int> nWins(N);
    int nTies = 0;
    std::vector<double> scores(N);

    ResultSimulation rs(0.5, 0.5);
    std::vector<std::vector<ResultSimulation>> rsM;

    std::vector<double> diffV { 0,   50,  100, 150, 200, 250,  300,  400 };
    std::vector<double> drawV { .64, .60, .52, .45, .38, .343, .282, .176 };
    for (int i = 0; i < N; i++) {
        std::vector<ResultSimulation> tmp;
        for (int j = 0; j < N; j++) {
            double eloDiff = abs(elo[i] - elo[j]);
#if 0
            double mean = ResultSimulation::eloToResult(elo[i] - elo[j]);
            double drawProb = std::min(mean, 1 - mean) * 0.8;
            for (int k = 1; k < (int)diffV.size(); k++) {
                if (eloDiff <= diffV[k]) {
                    drawProb = drawV[k-1] + (drawV[k] - drawV[k-1]) * (eloDiff - diffV[k-1]) / (diffV[k] - diffV[k-1]);
                    break;
                }
            }
#else
            auto f = [](double delta) -> double { return 1/(1+pow(10, delta / 400)); };
            double eloDraw = 200;
            double winP = f(-eloDiff + eloDraw);
            double lossP = f(+eloDiff + eloDraw);
            double drawP = 1 - winP - lossP;
//            std::cout << "winP:" << winP << " lossP:" << lossP << " drawP:" << drawP << std::endl;
            double mean = winP + drawP / 2;
            double drawProb = drawP;
#endif
//            std::cout << "i:" << i << " j:" << j << " m:" << mean << " d:" << drawProb << std::endl;
            tmp.emplace_back(mean, drawProb);
        }
        rsM.push_back(tmp);
    }

    for (int s = 0; s < nSimul; s++) {
        for (int i = 0; i < N; i++)
            scores[i] = 0.0;
        for (int i = 0; i < N; i++) {
            for (int j = i+1; j < N; j++) {
//                double mean = ResultSimulation::eloToResult(elo[i] - elo[j]);
//                double drawProb = std::min(mean, 1 - mean) * 0.8;
//                rs.setParams(mean, drawProb);
//                double score = (nRounds == 1) ? rs.simulateOneGame()
//                                              : rs.simulate(nRounds);
                double score = (nRounds == 1) ? rsM[i][j].simulateOneGame()
                                              : rsM[i][j].simulate(nRounds);
//                std::cout << "i:" << i << " j:" << j << " score:" << score << std::endl;
                scores[i] += score;
                scores[j] += 1 - score;
            }
        }
//        for (int i = 0; i < N; i++)
//            std::cout << "i:" << i << " score:" << scores[i] << std::endl;
        int bestI = 0;
        double bestScore = scores[bestI];
        int nBest = 1;
        int secondBestI = -1;
        for (int i = 1; i < N; i++) {
            if (rint(scores[i]*2) > rint(bestScore*2)) {
                bestI = i;
                bestScore = scores[i];
                nBest = 1;
//                std::cout << "findBest i:" << i << " best:" << bestScore << std::endl;
            } else if (rint(scores[i]*2) == rint(bestScore*2)) {
                secondBestI = i;
                nBest++;
//                std::cout << "findBest i:" << i << " tie, nBest:" << nBest << std::endl;
            }
        }
        if (nBest == 1) {
            nWins[bestI]++;
        } else if (nBest == 2) {
            double score = rsM[bestI][secondBestI].simulate(2);
            if (score > 0.5)
                nWins[bestI]++;
            else if (score < 0.5)
                nWins[secondBestI]++;
            else
                nTies++;
        } else {
            nTies++;
        }
    }

    for (int i = 0; i < N; i++) {
        std::stringstream ss;
        ss.precision(6);
        ss << std::fixed << nWins[i] / (double)nSimul;
        std::cout << std::setw(2) << i + 1 << "  " << std::setw(4) << elo[i] << "  "
                  << std::setw(8) << ss.str() << std::endl;
//        std::cout << "i:" << i << " nWin:" << nWins[i] << " " << nWins[i] / (double)nSimul << std::endl;
    }
    std::cout << "ties:" << nTies << " " << nTies / (double)nSimul << std::endl;
}

void
Spsa::spsaSimulation(int nSimul, int nIter, int gamesPerIter, double a, double c,
                     const std::vector<double>& startParams) {
    const int N = startParams.size();
    std::vector<double> params(startParams);
    std::vector<double> paramsNeg(startParams);
    std::vector<double> paramsPos(startParams);
    std::vector<double> signVec(N);
    std::vector<double> accum(N);
    Random rnd;
    rnd.setSeed(seeder.nextU64());
    SimulatedEnginePair enginePair;

    std::cout << "Initial elo: " << enginePair.getElo(startParams) << std::endl;

    std::vector<double> eloErr;
    for (int s = 0; s < nSimul; s++) {
        params = startParams;
        double A = nIter * 0.1;
        double alpha = 0.602;
        double gamma = 0.101;
        for (int k = 0; k < nIter; k++) {
            double ak = a / pow(A + k + 1, alpha);
            double ck = c / pow(k + 1, gamma);
            if (s == 0 && k == 0) {
                double C = ak * 0.5 * (1 - 0.43)/sqrt(gamesPerIter)/(2*ck*ck);
                std::cout << "C: " << C << std::endl;
            }
#if 0
            for (int i = 0; i < N; i++) {
                signVec[i] = (rnd.nextU64() & 1) ? 1 : -1;
                paramsNeg[i] = params[i] - ck * signVec[i];
                paramsPos[i] = params[i] + ck * signVec[i];
            }
            enginePair.setParams(paramsPos, paramsNeg);
            double dy = -(enginePair.simulate(gamesPerIter) - 0.5);
            for (int i = 0; i < N; i++)
                params[i] = params[i] - ak * dy / (2 * ck * signVec[i]);
#else
            for (int i = 0; i < N; i++)
                accum[i] = 0;
            for (int g = 0; g < gamesPerIter; g++) {
                for (int i = 0; i < N; i++) {
                    signVec[i] = (rnd.nextU64() & 1) ? 1 : -1;
                    paramsNeg[i] = params[i] - ck * signVec[i];
                    paramsPos[i] = params[i] + ck * signVec[i];
                }
                enginePair.setParams(paramsPos, paramsNeg);
                double dy = -(enginePair.simulate(1) - 0.5);
                for (int i = 0; i < N; i++)
                    accum[i] += dy / (2 * ck * signVec[i]);
            }
            for (int i = 0; i < N; i++)
                params[i] = params[i] - ak * accum[i] / gamesPerIter;
#endif
            if (nSimul == 1) {
                if ((k == nIter - 1) || ((k % (std::max(nIter,500) / 500)) == 0))
                    std::cout << "k:" << k << " params: " << params
                              << " elo:" << enginePair.getElo(params) << std::endl;
            }
        }
        if (nSimul > 1) {
            if ((s == nSimul - 1) || ((s % (std::max(nSimul,20) / 20)) == 0))
                std::cout << "s:" << s << " params: " << params
                          << " elo:" << enginePair.getElo(params) << std::endl;
        }
        eloErr.push_back(enginePair.getElo(params));
//        std::cout << "eloErr: " << enginePair.getElo(params) << std::endl;
    }
    if (nSimul > 1) {
        double sum = 0;
        double sum2 = 0;
        double nElem = eloErr.size();
        for (int i = 0; i < nElem; i++) {
            double v = eloErr[i];
            sum += v;
            sum2 += v * v;
        }
        double mean = sum / nElem;
        double var = (sum2 - sum * sum / nElem) / (nElem - 1);
        double s = sqrt(var);
        std::cout << "eloAvg:" << mean << " eloStd:" << s << std::endl;
    }
}

// --------------------------------------------------------------------------------

class ParamDblValue {
public:
    ParamDblValue() : value(-1) {}
    std::string name;
    double value;
};

/** Calls an external script to run games. */
class GameRunner {
public:
    /** Constructor. */
    GameRunner(const std::string& script, const std::string& computer, int instanceNo);

    /** Run games and return the average score for engine 1. */
    double runGame(const std::vector<ParamDblValue>& engine1Params,
                   const std::vector<ParamDblValue>& engine2Params);

    std::string compName() const { return computer; }
    int instNo() const { return instanceNo; }

private:
    /** Round "value" up or down to an integer. The expected value of
     * the return value is equal to the original value. */
    int stochasticRound(double value);

    /** Run a script and return the script standard output as a string. */
    std::string runScript(const std::string& cmdLine);

    std::string script;
    std::string computer;
    int instanceNo;
    Random rnd;
};

GameRunner::GameRunner(const std::string& script0, const std::string& computer0,
                       int instanceNo0)
    : script(script0), computer(computer0), instanceNo(instanceNo0) {
    rnd.setSeed(seeder.nextU64());
}

double
GameRunner::runGame(const std::vector<ParamDblValue>& engine1Params,
                    const std::vector<ParamDblValue>& engine2Params) {
    std::string cmdLine = "\"" + script + "\" " + computer + " " + num2Str(instanceNo);
    for (const auto& p : engine1Params)
        cmdLine += " " + p.name + " " + num2Str(stochasticRound(p.value));
    cmdLine += " :";
    for (const auto& p : engine2Params)
        cmdLine += " " + p.name + " " + num2Str(stochasticRound(p.value));
    std::string result = runScript(cmdLine);
    std::vector<std::string> words;
    splitString(result, words);
    int win, loss, draw;
    if (words.size() != 3 || !str2Num(words[0], win) ||
            !str2Num(words[1], loss) || !str2Num(words[2], draw) ||
            win + loss + draw <= 0)
        throw ChessParseError("script return value error: '" + result + "'");
    return (win + draw * 0.5) / (win + loss + draw);
}

std::string
GameRunner::runScript(const std::string& cmdLine) {
    std::shared_ptr<FILE> f(popen(cmdLine.c_str(), "r"),
                            [](FILE* f) { pclose(f); });
    char buf[256];
    buf[0] = 0;
    fgets(buf, sizeof(buf), f.get());
    return std::string(buf);
}

int
GameRunner::stochasticRound(double value) {
    int ip = (int)floor(value);
    double fp = value - ip;
    double r = rnd.nextU64() / (double)std::numeric_limits<U64>::max();
    return (r < fp) ? ip + 1 : ip;
}

/** Handle scheduling of WorkUnits to GameRunners. */
class GameScheduler {
public:
    /** Constructor. */
    GameScheduler();

    /** Destructor. Waits for all threads to terminate. */
    ~GameScheduler();

    /** Add a GameRunner. */
    void addWorker(const GameRunner& gr);

    /** Start the worker threads. Create one thread for each GameRunner object. */
    void startWorkers();

    /** Wait for currently running WorkUnits to finish and then stop all threads. */
    void stopWorkers();

    struct WorkUnit {
        int id;
        std::vector<ParamDblValue> engine1Params;
        std::vector<ParamDblValue> engine2Params;
        std::vector<double> signVec;

        double result;         // Average score for engine 1
        std::string compName;  // Name of computer that run this WorkUnit
        int instNo;            // Instance number that run this WorkUnit
    };

    /** Add a WorkUnit to the queue. */
    void addWorkUnit(const WorkUnit& wu);

    /** Wait until a result is ready and retrieve the corresponding WorkUnit. */
    void getResult(WorkUnit& wu);

private:
    /** Worker thread main loop. */
    void workerLoop(GameRunner& gr);

    bool stopped;
    std::mutex mutex;

    std::vector<GameRunner> workers;
    std::vector<std::unique_ptr<std::thread>> threads;

    std::deque<WorkUnit> pending;
    std::condition_variable pendingCv;

    std::deque<WorkUnit> complete;
    std::condition_variable completeCv;
};

GameScheduler::GameScheduler()
    : stopped(false) {
}

GameScheduler::~GameScheduler() {
    stopWorkers();
}

void
GameScheduler::addWorker(const GameRunner& gr) {
    workers.push_back(gr);
}

void
GameScheduler::startWorkers() {
    for (GameRunner& gr : workers) {
        auto thread = make_unique<std::thread>([this,&gr]() {
            workerLoop(gr);
        });
        threads.push_back(std::move(thread));
    }
}

void
GameScheduler::stopWorkers() {
    {
        std::lock_guard<std::mutex> L(mutex);
        stopped = true;
        pendingCv.notify_all();
    }
    for (auto& t : threads) {
        t->join();
        t.reset();
    }
}

void
GameScheduler::addWorkUnit(const WorkUnit& wu) {
    std::lock_guard<std::mutex> L(mutex);
    bool empty = pending.empty();
    pending.push_back(wu);
    if (empty)
        pendingCv.notify_all();
}

void
GameScheduler::getResult(WorkUnit& wu) {
    std::unique_lock<std::mutex> L(mutex);
    while (complete.empty())
        completeCv.wait(L);
    wu = complete.front();
    complete.pop_front();
}

void
GameScheduler::workerLoop(GameRunner& gr) {
    while (true) {
        WorkUnit wu;
        {
            std::unique_lock<std::mutex> L(mutex);
            while (!stopped && pending.empty())
                pendingCv.wait(L);
            if (stopped)
                return;
            wu = pending.front();
            pending.pop_front();
        }
        wu.result = gr.runGame(wu.engine1Params, wu.engine2Params);
        wu.compName = gr.compName();
        wu.instNo = gr.instNo();
        {
            std::unique_lock<std::mutex> L(mutex);
            bool empty = complete.empty();
            complete.push_back(wu);
            if (empty)
                completeCv.notify_all();
        }
    }
}

/** Configuration parameters for SPSA optimization. */
class SpsaConfig {
public:
    /** Constructor. Read configuration from a file. */
    SpsaConfig(const std::string& filename);

    struct ParamData {
        std::string parName;
        double c0;
        double value;
        double minValue;
        double maxValue;
    };

    struct ComputerData {
        std::string compName;  // Host name
        int numInstances;      // Number of worker instances
    };

    std::string scriptName() const { return script; }
    int numGames() const { return nGames; }
    int gamesPerIter() const { return q; }
    double initialGain() const { return C; }
    const std::vector<ParamData>& params() const { return paramVec; }
    const std::vector<ComputerData>& computers() const { return computerVec; }

private:
    std::string script; // Name of external script
    int nGames;         // Nominal number of games
    int q;              // Number of games per iteration
    double C;           // Initial gain factor

    std::vector<ParamData> paramVec;
    std::vector<ComputerData> computerVec;
};

SpsaConfig::SpsaConfig(const std::string& filename)
    : nGames(0), q(0), C(0) {
    std::ifstream is(filename);
    while (true) {
        std::string line;
        std::getline(is, line);
        if (!is.good())
            break;
        line = trim(line);
        if (line.length() <= 0 || line[0] == '#')
            continue;
        std::vector<std::string> words;
        splitString(line, words);
        auto error = [&line]() { throw ChessParseError("Error in config file: " + line); };
        const int nWords = words.size();
        if (nWords < 2)
            error();
        std::string key = toLowerCase(words[0]);
        if (key == "script") {
            if (nWords != 2)
                error();
            script = words[1];
        } else if (key == "numgames") {
            int tmp;
            if (nWords != 2 || !str2Num(words[1], tmp))
                error();
            nGames = tmp;
        } else if (key == "q") {
            int tmp;
            if (nWords != 2 || !str2Num(words[1], tmp))
                error();
            q = (tmp + 1) / 2 * 2;
        } else if (key == "c") {
            double tmp;
            if (nWords != 2 || !str2Num(words[1], tmp))
                error();
            C = tmp;
        } else if (key == "parameter") {
            if (nWords != 3 && nWords != 6)
                error();
            ParamData pd;
            pd.parName = words[1];
            if (!str2Num(words[2], pd.c0) || (pd.c0 <= 0))
                error();
            if (nWords == 6) {
                if (!str2Num(words[3], pd.value) ||
                        !str2Num(words[4], pd.minValue) ||
                        !str2Num(words[5], pd.maxValue))
                    error();
            } else {
                Parameters& uciPars = Parameters::instance();
                std::shared_ptr<Parameters::ParamBase> par = uciPars.getParam(pd.parName);
                if (!par)
                    throw ChessParseError("No such parameter: " + pd.parName);
                auto sPar = dynamic_cast<Parameters::SpinParam*>(par.get());
                if (!sPar)
                    throw ChessParseError("Incorrect parameter type: " + pd.parName);
                pd.value = sPar->getDefaultValue();
                pd.minValue = sPar->getMinValue();
                pd.maxValue = sPar->getMaxValue();
            }
            paramVec.push_back(pd);
        } else if (key == "computer") {
            ComputerData cd;
            if (nWords != 3 || !str2Num(words[2], cd.numInstances) || (cd.numInstances <= 0))
                error();
            cd.compName = words[1];
            computerVec.push_back(cd);
        } else {
            error();
        }
    }
    if (nGames < q || q <= 0 || C <= 0)
        throw ChessParseError("Error in config file");
    if (computerVec.empty())
        throw ChessParseError("No computers defined");
    if (paramVec.empty())
        throw ChessParseError("No parameters defined");
}

void
Spsa::spsa(const std::string& configFile) {
    GameScheduler gs;
    SpsaConfig conf(configFile);
    std::cout << "script: " << conf.scriptName() << std::endl;
    std::cout << "nGames: " << conf.numGames() << std::endl;
    std::cout << "q     : " << conf.gamesPerIter() << std::endl;
    std::cout << "C     : " << conf.initialGain() << std::endl;
    for (const SpsaConfig::ParamData& pd : conf.params())
        std::cout << "param: " << pd.parName << " c0: " << pd.c0
                  << " start: " << pd.value << " min: " << pd.minValue
                  << " max: " << pd.maxValue << std::endl;
    for (const SpsaConfig::ComputerData& cd : conf.computers()) {
        std::cout << "computer: " << cd.compName << " nInst: " << cd.numInstances << std::endl;
        for (int i = 0; i < cd.numInstances; i++) {
            GameRunner gr(conf.scriptName(), cd.compName, i+1);
            gs.addWorker(gr);
        }
    }

    gs.startWorkers();

    std::vector<SpsaConfig::ParamData> parInfo = conf.params();
    const int gamesPerIter = conf.gamesPerIter();
    const int q = gamesPerIter / 2;
    const double C = conf.initialGain();
    const int nIter = conf.numGames() / gamesPerIter;

    const double A = nIter * 0.1;
    const double alpha = 0.602;
    const double gamma = 0.101;
    const double drawRate = 0.43;
    const double a = 4 * C * pow(A + 1, alpha) / (1 - drawRate) * sqrt(q*2);

    const int N = parInfo.size();
    std::vector<double> startValue(N);
    std::vector<double> accum(N);
    Random rnd;
    rnd.setSeed(seeder.nextU64());

    for (int i = 0; i < N; i++) {
        startValue[i] = parInfo[i].value;
        std::cout << parInfo[i].parName << " " << parInfo[i].value << std::endl;
    }

    const double t0 = currentTime();
    for (int k = 0; (k < nIter) || true; k++) {
        double ak = a / pow(A + k + 1, alpha);
        double ck = 1.0 / pow(k + 1, gamma);
        for (int i = 0; i < N; i++)
            accum[i] = 0;
        for (int g = 0; g < q; g++) {
            GameScheduler::WorkUnit wu;
            wu.id = g;
            wu.signVec.resize(N);
            wu.engine1Params.resize(N);
            wu.engine2Params.resize(N);
            for (int i = 0; i < N; i++) {
                wu.signVec[i] = (rnd.nextU64() & 1) ? 1 : -1;
                wu.engine1Params[i].name = parInfo[i].parName;
                wu.engine2Params[i].name = parInfo[i].parName;
                double c0 = parInfo[i].c0;
                wu.engine1Params[i].value = clamp(parInfo[i].value + c0 * ck * wu.signVec[i],
                                                  parInfo[i].minValue, parInfo[i].maxValue);
                wu.engine2Params[i].value = clamp(parInfo[i].value - c0 * ck * wu.signVec[i],
                                                  parInfo[i].minValue, parInfo[i].maxValue);
            }
            gs.addWorkUnit(wu);
        }
        for (int g = 0; g < q; g++) {
            GameScheduler::WorkUnit wu;
            gs.getResult(wu);
            double dy = -(wu.result - 0.5);
            for (int i = 0; i < N; i++)
                accum[i] += dy / (2 * ck * wu.signVec[i]);
        }
        std::cout << "Iteration: " << k << " ak:" << ak << " ck:" << ck << " time:" << currentTime() - t0
                  << " games:" << (k+1)*gamesPerIter << std::endl;
        for (int i = 0; i < N; i++) {
            parInfo[i].value -= ak * parInfo[i].c0 * accum[i] / q;
            std::cout << parInfo[i].parName << " " << parInfo[i].value << " "
                      << startValue[i] << " *" << std::endl;
        }
    }
}

// --------------------------------------------------------------------------------

ResultSimulation::ResultSimulation(double meanResult, double drawProb)
    : rng(gsl_rng_alloc(gsl_rng_mt19937),
          [](gsl_rng* rng) { gsl_rng_free(rng); })
{
    U64 s = seeder.nextU64();
//    std::cout << "seed:" << s << std::endl;
    gsl_rng_set(rng.get(), s);
    rnd.setSeed(seeder.nextU64());
    setParams(meanResult, drawProb);
}

void
ResultSimulation::setParams(double meanResult, double drawProb) {
    drawP = drawProb;
    winP = meanResult - drawP * 0.5;
    lossP = 1 - winP - drawP;
    assert(winP >= 0);
    assert(winP <= 1);
    assert(drawP >= 0);
    assert(drawP <= 1);
    assert(lossP >= 0);
    assert(lossP <= 1);
}

double
ResultSimulation::simulate(int nGames) {
    int nWin, nDraw, nLoss;
    simulate(nGames, nWin, nDraw, nLoss);
    return (nWin + nDraw * 0.5) / nGames;
}

void
ResultSimulation::simulate(int nGames, int& nWin, int& nDraw, int& nLoss) {
    double p[3];
    unsigned int n[3];
    p[0] = winP;
    p[1] = drawP;
    p[2] = lossP;

    gsl_ran_multinomial(rng.get(), 3, nGames, &p[0], &n[0]);
    nWin = n[0];
    nDraw = n[1];
    nLoss = n[2];
}

double
ResultSimulation::simulateOneGame() {
    double r = rnd.nextU64() / (double)std::numeric_limits<U64>::max();
    if (r < lossP)
        return 0;
    else if (r < lossP + drawP)
        return 0.5;
    else
        return 1;
}

// --------------------------------------------------------------------------------

SimulatedEnginePair::SimulatedEnginePair()
    : rs(0.5, 0.4) {
}

void
SimulatedEnginePair::setParams(const std::vector<double>& params1,
                               const std::vector<double>& params2) {
    double eloa = getElo(params1);
    double elob = getElo(params2);
    double elo = eloa - elob;
    double mean = ResultSimulation::eloToResult(elo);
    double drawProb = std::min(mean, 1 - mean) * 0.8;
    rs.setParams(mean, drawProb);
}

double
SimulatedEnginePair::simulate(int nGames) {
    if (nGames == 1)
        return rs.simulateOneGame();
    else
        return rs.simulate(nGames);
}

double
SimulatedEnginePair::getElo(const std::vector<double>& params) {
    auto sqr = [](double x) -> double { return x * x; };
#if 1
    assert(params.size() == 3);
    double p0 = params[0];
    double p1 = params[1];
    double p2 = params[2];
    double elo = 0 - sqr(p0 - 120) / 400 * 20
                   - sqr(p1 - 65) / 400 * 10;
    double px=std::max(p2*(1.0/10)-10,0.0);
    elo += 10*(px*px*exp(-px)-4*exp(-2));
    return elo / 10;
#else
    assert(params.size() == 10);
    double elo = 0;
    for (int i = 0; i < 10; i++) {
        double c = 110 + i * 2;
        elo -= sqr(params[i] - c) / 400 * 2;
    }
    return elo;
#endif
}
