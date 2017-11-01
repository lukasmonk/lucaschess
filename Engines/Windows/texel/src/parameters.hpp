/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * parameters.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef PARAMETERS_HPP_
#define PARAMETERS_HPP_

#include "util/util.hpp"
#include "piece.hpp"
#include "position.hpp"

#include <memory>
#include <functional>
#include <map>
#include <string>
#include <cassert>


/** Handles all UCI parameters. */
class Parameters {
public:
    enum Type {
        CHECK,
        SPIN,
        COMBO,
        BUTTON,
        STRING
    };

    /** Observer pattern. */
    class Listener {
        using Func = std::function<void()>;
    public:
        int addListener(Func f, bool callNow = true);

        void removeListener(int id);

    protected:
        void notify();

    private:
        int nextId = 0;
        std::map<int, Func> listeners;
    };

    /** Base class for UCI parameters. */
    struct ParamBase : public Listener {
        std::string name;
        Type type;

        ParamBase(const std::string& n, Type t) : name(n), type(t) { }
        ParamBase(const ParamBase& other) = delete;
        ParamBase& operator=(const ParamBase& other) = delete;

        virtual bool getBoolPar() const { assert(false); return false; }
        virtual int getIntPar() const { assert(false); return 0; }
        virtual std::string getStringPar() const { assert(false); return ""; }
        virtual void set(const std::string& value) { assert(false); }
    };

    /** A boolean parameter. */
    struct CheckParam : public ParamBase {
        bool value;
        bool defaultValue;

        CheckParam(const std::string& name, bool def)
            : ParamBase(name, CHECK) {
            this->value = def;
            this->defaultValue = def;
        }

        bool getBoolPar() const override { return value; }

        void set(const std::string& value) override {
            if (toLowerCase(value) == "true")
                this->value = true;
            else if (toLowerCase(value) == "false")
                this->value = false;
            notify();
        }
    };

    /** An integer parameter. */
    struct SpinParam : public ParamBase {
        int minValue;
        int maxValue;
        int value;
        int defaultValue;

        SpinParam(const std::string& name, int minV, int maxV, int def)
            : ParamBase(name, SPIN) {
            minValue = minV;
            maxValue = maxV;
            value = def;
            defaultValue = def;
        }

        int getIntPar() const override { return value; }

        void set(const std::string& value) override {
            int val;
            if (str2Num(value, val) && (val >= minValue) && (val <= maxValue)) {
                this->value = val;
                notify();
            }
        }

        int getDefaultValue() const { return defaultValue; }
        int getMinValue() const { return minValue; }
        int getMaxValue() const { return maxValue; }
    };

    /** A multi-choice parameter. */
    struct ComboParam : public ParamBase {
        std::vector<std::string> allowedValues;
        std::string value;
        std::string defaultValue;

        ComboParam(const std::string& name, const std::vector<std::string>& allowed,
                   const std::string& def)
            : ParamBase(name, COMBO), allowedValues(allowed),
              value(def), defaultValue(def) {
        }

        std::string getStringPar() const override { return value; }

        void set(const std::string& value) override {
            for (size_t i = 0; i < allowedValues.size(); i++) {
                const std::string& allowed = allowedValues[i];
                if (toLowerCase(allowed) == toLowerCase(value)) {
                    this->value = allowed;
                    notify();
                    break;
                }
            }
        }
    };

    /** An action parameter. */
    struct ButtonParam : public ParamBase {
        explicit ButtonParam(const std::string& name)
            : ParamBase(name, BUTTON) { }

        void set(const std::string& value) override {
            notify();
        }
    };

    /** A string parameter. */
    struct StringParam : public ParamBase {
        std::string value;
        std::string defaultValue;

        StringParam(const std::string& name, const std::string& def)
            : ParamBase(name, STRING), value(def), defaultValue(def) {
        }

        std::string getStringPar() const override { return value; }

        void set(const std::string& value) override {
            this->value = value;
            notify();
        }
    };

    /** Get singleton instance. */
    static Parameters& instance();

    /** Retrieve list of all parameters. */
    void getParamNames(std::vector<std::string>& parNames);

    std::shared_ptr<ParamBase> getParam(const std::string& name) const;

    bool getBoolPar(const std::string& name) const;
    int getIntPar(const std::string& name) const;
    std::string getStringPar(const std::string& name) const;

    void set(const std::string& name, const std::string& value);

    /** Register a parameter. */
    void addPar(const std::shared_ptr<ParamBase>& p);

private:
    Parameters();

    std::map<std::string, std::shared_ptr<ParamBase>> params;
    std::vector<std::string> paramNames;
};

// ----------------------------------------------------------------------------

/** Param can be either a UCI parameter or a compile time constant. */
template <int defaultValue, int minValue, int maxValue, bool uci> class Param;

template <int defaultValue, int minValue, int maxValue>
class Param<defaultValue, minValue, maxValue, false> {
public:
    Param() {}
    operator int() const { return defaultValue; }
    void registerParam(const std::string& name, Parameters& pars) {}
    template <typename Func> void addListener(Func f) { f(); }
};

template <int defaultValue, int minValue, int maxValue>
class Param<defaultValue, minValue, maxValue, true> {
public:
    Param() : value(0) {}
    operator int() const { return value; }
    void registerParam(const std::string& name, Parameters& pars) {
        par = std::make_shared<Parameters::SpinParam>(name, minValue, maxValue, defaultValue);
        pars.addPar(par);
        par->addListener([this](){ value = par->getIntPar(); });
    }
    template <typename Func> void addListener(Func f) {
        if (par)
            par->addListener(f, false);
        f();
    }
private:
    int value;
    std::shared_ptr<Parameters::SpinParam> par;
};

#define DECLARE_PARAM(name, defV, minV, maxV, uci) \
    using name##ParamType = Param<defV,minV,maxV,uci>; \
    extern name##ParamType name;

#define DEFINE_PARAM(name) \
    name##ParamType name;

#define REGISTER_PARAM(varName, uciName) \
    varName.registerParam(uciName, *this);

// ----------------------------------------------------------------------------

/** Non-template base class to reduce executable code size. */
class ParamTableBase : public Parameters::Listener {
public:
    int getMinValue() const { return minValue; }
    int getMaxValue() const { return maxValue; }

protected:
    ParamTableBase(bool uci0, int minVal0, int maxVal0) :
        uci(uci0), minValue(minVal0), maxValue(maxVal0) {}
    void registerParamsN(const std::string& name, Parameters& pars,
                         int* table, int* parNo, int N);
    void modifiedN(int* table, int* parNo, int N);

    const bool uci;
    const int minValue;
    const int maxValue;

    std::vector<std::shared_ptr<Parameters::SpinParam>> params;
};

template <int N>
class ParamTable : public ParamTableBase {
public:
    ParamTable(int minVal, int maxVal, bool uci,
               std::initializer_list<int> data,
               std::initializer_list<int> parNo);

    int operator[](int i) const { return table[i]; }
    const int* getTable() const { return table; }

    void registerParams(const std::string& name, Parameters& pars) {
        registerParamsN(name, pars, table, parNo, N);
    }
private:
    int table[N];
    int parNo[N];
};

template <int N>
class ParamTableMirrored {
public:
    explicit ParamTableMirrored(ParamTable<N>& orig0) : orig(orig0) {
        orig.addListener([this]() {
            if (N == 64) {
                for (int sq = 0; sq < N; sq++)
                    table[sq] = orig[Position::mirrorY(sq)];
            } else {
                for (int i = 0; i < N; i++)
                    table[i] = orig[N-1-i];
            }
        });
    }
    int operator[](int i) const { return table[i]; }
    const int* getTable() const { return table; }
private:
    int table[N];
    ParamTable<N>& orig;
};

template <int N>
ParamTable<N>::ParamTable(int minVal0, int maxVal0, bool uci0,
                          std::initializer_list<int> table0,
                          std::initializer_list<int> parNo0)
    : ParamTableBase(uci0, minVal0, maxVal0) {
    assert(table0.size() == N);
    assert(parNo0.size() == N);
    for (int i = 0; i < N; i++) {
        table[i] = table0.begin()[i];
        parNo[i] = parNo0.begin()[i];
    }
}

inline bool
Parameters::getBoolPar(const std::string& name) const {
    return getParam(name)->getBoolPar();
}

inline int
Parameters::getIntPar(const std::string& name) const {
    return getParam(name)->getIntPar();
}

inline std::string
Parameters::getStringPar(const std::string& name) const {
    return getParam(name)->getStringPar();
}

inline void
Parameters::set(const std::string& name, const std::string& value) {
    auto it = params.find(toLowerCase(name));
    if (it == params.end())
        return;
    it->second->set(value);
}

// ----------------------------------------------------------------------------
// UCI parameters

namespace UciParams {
    extern std::shared_ptr<Parameters::SpinParam> hash;
    extern std::shared_ptr<Parameters::CheckParam> ownBook;
    extern std::shared_ptr<Parameters::StringParam> bookFile;
    extern std::shared_ptr<Parameters::CheckParam> ponder;
    extern std::shared_ptr<Parameters::CheckParam> analyseMode;
    extern std::shared_ptr<Parameters::SpinParam> strength;
    extern std::shared_ptr<Parameters::SpinParam> threads;
    extern std::shared_ptr<Parameters::SpinParam> multiPV;

    extern std::shared_ptr<Parameters::CheckParam> useNullMove;

    extern std::shared_ptr<Parameters::StringParam> gtbPath;
    extern std::shared_ptr<Parameters::SpinParam> gtbCache;
    extern std::shared_ptr<Parameters::StringParam> rtbPath;
    extern std::shared_ptr<Parameters::SpinParam> minProbeDepth;

    extern std::shared_ptr<Parameters::CheckParam> analysisAgeHash;
    extern std::shared_ptr<Parameters::ButtonParam> clearHash;
}

// ----------------------------------------------------------------------------
// Tuning parameters

const bool useUciParam = false;

extern int pieceValue[Piece::nPieceTypes];

// Evaluation parameters

DECLARE_PARAM(pV, 100, 1, 200, useUciParam);
DECLARE_PARAM(nV, 398, 1, 800, useUciParam);
DECLARE_PARAM(bV, 398, 1, 800, useUciParam);
DECLARE_PARAM(rV, 607, 1, 1200, useUciParam);
DECLARE_PARAM(qV, 1254, 1, 2400, useUciParam);
DECLARE_PARAM(kV, 9900, 9900, 9900, false); // Used by SEE algorithm but not included in board material sums

DECLARE_PARAM(pawnIslandPenalty,        7, 0, 50, useUciParam);
DECLARE_PARAM(pawnBackwardPenalty,      13, 0, 50, useUciParam);
DECLARE_PARAM(pawnSemiBackwardPenalty1, 11, -50, 50, useUciParam);
DECLARE_PARAM(pawnSemiBackwardPenalty2, 2, -50, 50, useUciParam);
DECLARE_PARAM(pawnRaceBonus,            167, 0, 1000, useUciParam);
DECLARE_PARAM(passedPawnEGFactor,       62, 1, 128, useUciParam);
DECLARE_PARAM(RBehindPP1,               12, -100, 100, useUciParam);
DECLARE_PARAM(RBehindPP2,               26, -100, 100, useUciParam);
DECLARE_PARAM(activePawnPenalty,         6, -50, 50, useUciParam);

DECLARE_PARAM(QvsRMBonus1,         43, -100, 100, useUciParam);
DECLARE_PARAM(QvsRMBonus2,         -23, -100, 100, useUciParam);
DECLARE_PARAM(knightVsQueenBonus1, 125, 0, 200, useUciParam);
DECLARE_PARAM(knightVsQueenBonus2, 251, 0, 600, useUciParam);
DECLARE_PARAM(knightVsQueenBonus3, 453, 0, 800, useUciParam);
DECLARE_PARAM(krkpBonus,           107, 0, 400, useUciParam);
DECLARE_PARAM(krpkbBonus,          131, -200, 200, useUciParam);
DECLARE_PARAM(krpkbPenalty,        69,  0, 128, useUciParam);
DECLARE_PARAM(krpknBonus,          149, 0, 400, useUciParam);
DECLARE_PARAM(RvsBPBonus,          -30, -200, 200, useUciParam);

DECLARE_PARAM(pawnTradePenalty,    50, 0, 100, useUciParam);
DECLARE_PARAM(pawnTradeThreshold,  358, 100, 1000, useUciParam);

DECLARE_PARAM(threatBonus1,       66, 5, 500, useUciParam);
DECLARE_PARAM(threatBonus2,     1196, 100, 10000, useUciParam);
DECLARE_PARAM(latentAttackBonus,  17, -50, 50, useUciParam);

DECLARE_PARAM(rookHalfOpenBonus,     16, 0, 100, useUciParam);
DECLARE_PARAM(rookOpenBonus,         26, 0, 100, useUciParam);
DECLARE_PARAM(rookDouble7thRowBonus, 88, 0, 100, useUciParam);
DECLARE_PARAM(trappedRookPenalty1,   78, 0, 200, useUciParam);
DECLARE_PARAM(trappedRookPenalty2,   48, 0, 200, useUciParam);

DECLARE_PARAM(bishopPairPawnPenalty, 5, 0, 10, useUciParam);
DECLARE_PARAM(trappedBishopPenalty,  141, 0, 300, useUciParam);
DECLARE_PARAM(oppoBishopPenalty,     81, 0, 128, useUciParam);

DECLARE_PARAM(kingSafetyHalfOpenBCDEFG1, 12, 0, 100, useUciParam);
DECLARE_PARAM(kingSafetyHalfOpenBCDEFG2, 2, -50, 100, useUciParam);
DECLARE_PARAM(kingSafetyHalfOpenAH1,     18, 0, 100, useUciParam);
DECLARE_PARAM(kingSafetyHalfOpenAH2,     10, 0, 100, useUciParam);
DECLARE_PARAM(kingSafetyWeight1,         28, -50, 200, useUciParam);
DECLARE_PARAM(kingSafetyWeight2,         -46, -50, 200, useUciParam);
DECLARE_PARAM(kingSafetyWeight3,         4, -50, 200, useUciParam);
DECLARE_PARAM(kingSafetyWeight4,         0, -50, 200, useUciParam);
DECLARE_PARAM(kingSafetyThreshold,       44, 0, 200, useUciParam);
DECLARE_PARAM(knightKingProtectBonus,    13, -50, 50, useUciParam);
DECLARE_PARAM(bishopKingProtectBonus,    11, -50, 50, useUciParam);
DECLARE_PARAM(pawnStormBonus,            12, 0, 20, useUciParam);

DECLARE_PARAM(pawnLoMtrl,          1007, 0, 10000, useUciParam);
DECLARE_PARAM(pawnHiMtrl,          6415, 0, 10000, useUciParam);
DECLARE_PARAM(minorLoMtrl,         2228, 0, 10000, useUciParam);
DECLARE_PARAM(minorHiMtrl,         7492, 0, 10000, useUciParam);
DECLARE_PARAM(castleLoMtrl,        713, 0, 10000, useUciParam);
DECLARE_PARAM(castleHiMtrl,        7884, 0, 10000, useUciParam);
DECLARE_PARAM(queenLoMtrl,         4593, 0, 10000, useUciParam);
DECLARE_PARAM(queenHiMtrl,         6596, 0, 10000, useUciParam);
DECLARE_PARAM(passedPawnLoMtrl,    776, 0, 10000, useUciParam);
DECLARE_PARAM(passedPawnHiMtrl,    2512, 0, 10000, useUciParam);
DECLARE_PARAM(kingSafetyLoMtrl,    917, 0, 10000, useUciParam);
DECLARE_PARAM(kingSafetyHiMtrl,    3564, 0, 10000, useUciParam);
DECLARE_PARAM(oppoBishopLoMtrl,    753, 0, 10000, useUciParam);
DECLARE_PARAM(oppoBishopHiMtrl,    3387, 0, 10000, useUciParam);
DECLARE_PARAM(knightOutpostLoMtrl, 296, 0, 10000, useUciParam);
DECLARE_PARAM(knightOutpostHiMtrl, 1082, 0, 10000, useUciParam);

extern ParamTable<64>         kt1b, kt2b, pt1b, pt2b, nt1b, nt2b, bt1b, bt2b, qt1b, qt2b, rt1b;
extern ParamTableMirrored<64> kt1w, kt2w, pt1w, pt2w, nt1w, nt2w, bt1w, bt2w, qt1w, qt2w, rt1w;

extern ParamTable<64> knightOutpostBonus;
extern ParamTable<64> protectedPawnBonus, attackedPawnBonus;
extern ParamTable<4> protectBonus;

extern ParamTable<15> rookMobScore;
extern ParamTable<14> bishMobScore;
extern ParamTable<28> knightMobScore;
extern ParamTable<28> queenMobScore;

extern ParamTable<36> connectedPPBonus;
extern ParamTable<8> passedPawnBonusX, passedPawnBonusY;
extern ParamTable<10> ppBlockerBonus;
extern ParamTable<8> candidatePassedBonus;

extern ParamTable<16> majorPieceRedundancy;
extern ParamTable<5> QvsRRBonus;
extern ParamTable<7> RvsMBonus, RvsMMBonus;
extern ParamTable<4> bishopPairValue;
extern ParamTable<7> rookEGDrawFactor, RvsBPDrawFactor;

extern ParamTable<4> castleFactor;
extern ParamTable<9> pawnShelterTable, pawnStormTable;
extern ParamTable<14> kingAttackWeight;
extern ParamTable<5> kingPPSupportK;
extern ParamTable<8> kingPPSupportP;
extern ParamTable<5> qContactCheckBonus;
extern ParamTable<7> pieceKingAttackBonus;

extern ParamTable<8> pawnDoubledPenalty;
extern ParamTable<8> pawnIsolatedPenalty;
extern ParamTable<10> halfMoveFactor;
extern ParamTable<9> stalePawnFactor;


// Search parameters

DECLARE_PARAM(aspirationWindow, 15, 1, 100, useUciParam);
DECLARE_PARAM(rootLMRMoveCount, 2, 0, 100, useUciParam);

DECLARE_PARAM(razorMargin1, 86, 1, 500, useUciParam);
DECLARE_PARAM(razorMargin2, 353, 1, 1000, useUciParam);

DECLARE_PARAM(reverseFutilityMargin1, 102, 1, 1000, useUciParam);
DECLARE_PARAM(reverseFutilityMargin2, 210, 1, 1000, useUciParam);
DECLARE_PARAM(reverseFutilityMargin3, 267, 1, 2000, useUciParam);
DECLARE_PARAM(reverseFutilityMargin4, 394, 1, 3000, useUciParam);

DECLARE_PARAM(futilityMargin1,  61, 1,  500, useUciParam);
DECLARE_PARAM(futilityMargin2, 144, 1,  500, useUciParam);
DECLARE_PARAM(futilityMargin3, 268, 1, 1000, useUciParam);
DECLARE_PARAM(futilityMargin4, 334, 1, 1000, useUciParam);

DECLARE_PARAM(lmpMoveCountLimit1,  3, 1, 256, useUciParam);
DECLARE_PARAM(lmpMoveCountLimit2,  6, 1, 256, useUciParam);
DECLARE_PARAM(lmpMoveCountLimit3, 12, 1, 256, useUciParam);
DECLARE_PARAM(lmpMoveCountLimit4, 24, 1, 256, useUciParam);

DECLARE_PARAM(lmrMoveCountLimit1,  3, 1, 256, useUciParam);
DECLARE_PARAM(lmrMoveCountLimit2, 12, 1, 256, useUciParam);

DECLARE_PARAM(quiesceMaxSortMoves, 8, 0, 256, useUciParam);
DECLARE_PARAM(deltaPruningMargin, 152, 0, 1000, useUciParam);


// Time management parameters

DECLARE_PARAM(timeMaxRemainingMoves, 35, 2, 200, useUciParam);
DECLARE_PARAM(bufferTime, 1000, 1, 10000, true);
DECLARE_PARAM(minTimeUsage, 85, 1, 100, useUciParam);
DECLARE_PARAM(maxTimeUsage, 350, 100, 1000, useUciParam);
DECLARE_PARAM(timePonderHitRate, 35, 1, 100, useUciParam);


#endif /* PARAMETERS_HPP_ */
