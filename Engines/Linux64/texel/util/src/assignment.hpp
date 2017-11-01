/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * assignment.hpp
 *
 *  Created on: Aug 30, 2015
 *      Author: petero
 */

#ifndef ASSIGNMENT_HPP_
#define ASSIGNMENT_HPP_

#include <vector>
#include <cassert>
#include <limits>


template<typename T>
class Matrix {
public:
    Matrix(int rows, int cols);

    /** Element access operators. */
    const T& operator()(int row, int col) const;
    T& operator()(int row, int col);

    int numRows() const;
    int numCols() const;

    void printMatrix(bool negate) const;

private:
    std::vector<T> data_;
    int rows_;
    int cols_;
};


template <typename WeightType>
class Assignment {
public:
    /**
     * Create a weighted assignment problem instance with weight matrix 'w'.
     */
    Assignment(const Matrix<WeightType>& w);

    /** Create an empty assignment problem instance. */
    Assignment();

    /**
     * Find a minimum weight matching.
     * Time complexity: O(N^3)
     */
    const std::vector<int>& optWeightMatch();

    /**
     * Change one entry in the cost matrix.
     * Calling optWeightMatch() again after modifying the cost matrix
     * will compute an incremental solution. This will typically be
     * much faster than recomputing the solution from scratch.
     */
    void setCost(int i, int j, WeightType w);

    /** Return cost of entry (i,j). */
    WeightType getCost(int i, int j) const;
    WeightType operator()(int i, int j) const;

    int getSize() const;

    struct State {
        std::vector<WeightType> ly;
        std::vector<int> Mx;
    };

    // Set/get solution state. Setting a good state can make the
    // optWeightMatch() function run faster.
    const State& getState() const;
    void setState(const State& state);

    // Print cost matrix to stdout
    void printMatrix() const { w_.printMatrix(true); }

private:
    Matrix<WeightType> w_;

    // Saved solution from last call to optWeightMatch()
    State state_;

    // Variables used in optWeightMatch
    std::vector<WeightType> lx;
    std::vector<int> My;
    std::vector<bool> S, T;
    std::vector<int> NlS;
    std::vector<int> slackY;
    std::vector<int> slackArgMin;
    std::vector<int> Mx2, My2;
};


template<typename T>
inline Matrix<T>::Matrix(int rows, int cols)
    : rows_(rows), cols_(cols) {
    const int size = rows_ * cols_;
    data_.resize(size);
    for (int i = 0; i < size; i++)
        data_[i] = 0;
}

template<typename T>
inline const T& Matrix<T>::operator()(int row, int col) const {
    return data_[row * cols_ + col];
}

template<typename T>
inline T& Matrix<T>::operator()(int row, int col) {
    return data_[row * cols_ + col];
}

template<typename T>
inline int Matrix<T>::numRows() const {
    return rows_;
}

template<typename T>
inline int Matrix<T>::numCols() const {
    return cols_;
}

template <typename T>
void
Matrix<T>::printMatrix(bool negate) const {
    int s = negate ? -1 : 1;
    for (int i = 0; i < numRows(); i++) {
	for (int j = 0; j < numCols(); j++)
            std::cout << ' ' << std::setw(4) << s*(*this)(i, j);
        std::cout << std::endl;
    }
}


template <typename WeightType>
inline const typename Assignment<WeightType>::State&
Assignment<WeightType>::getState() const {
    return state_;
}

template <typename WeightType>
inline void
Assignment<WeightType>::setState(const State& state) {
    state_ = state;
}

template <typename WeightType>
inline WeightType
Assignment<WeightType>::operator()(int i, int j) const {
    return getCost(i, j);
}

template <typename WeightType>
Assignment<WeightType>::Assignment(const Matrix<WeightType>& w)
    : w_(w) {
    assert(w.numRows() >= 0);
    assert(w.numRows() == w.numCols());

    const int N = w_.numRows();
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            w_(i,j) = -w_(i,j);
        }
    }

    state_.ly = std::vector<WeightType>(N, 0);
    state_.Mx = std::vector<int>(N, -1);

    lx.resize(N);
    My.resize(N);
    S.resize(N);
    T.resize(N);
    NlS.resize(N);
    slackY.resize(N);
    slackArgMin.resize(N);
    Mx2.resize(N);
    My2.resize(N);
}

template <typename WeightType>
Assignment<WeightType>::Assignment()
    : Assignment(Matrix<WeightType>(0, 0)) {
}

/**
 * This function implements the hungarian method.
 */
template <typename WeightType>
const std::vector<int>&
Assignment<WeightType>::optWeightMatch() {
    const int N = w_.numRows();

    // The labeling function
    lx.assign(N, 0);
    std::vector<WeightType>& ly(state_.ly);

    // The matching. -1 means unmatched.
    // Mx maps from source to target vertexes, and My maps in the other direction.
    // That is: My(Mx(x)) = x
    std::vector<int>& Mx(state_.Mx);
    My.assign(N, -1);

    // The S and T sets
    S.assign(N, false);
    T.assign(N, false);

    // Neighborhood to S induced by the lx/ly labeling
    // Indexed by target vertex. Contains corresponding source vertex,
    // or -1 if not in the neighborhood.
    NlS.assign(N, -1);

    // Indexed by target vertex. min(x in S, lx(x) + ly(y) - w(x,y))
    slackY.assign(N, 0);

    // Contains an x that minimizes the slackY expression.
    slackArgMin.assign(N, -1);

    // Compute initial feasible labeling.
    for (int x = 0; x < N; x++) {
        WeightType maxVal = w_(x, 0) - ly[0];
        for (int y = 1; y < N; y++) {
            maxVal = std::max(maxVal, w_(x, y) - ly[y]);
        }
        lx[x] = maxVal;
    }

    // Strengthen the ly labeling. This isn't necessary for correctness,
    // but often speeds up the algorithm in practice.
    for (int y = 0; y < N; y++) {
        WeightType minSlack = lx[0] - w_(0, y);
        for (int x = 1; x < N; x++) {
            WeightType slack = lx[x] - w_(x, y);
            minSlack = std::min(minSlack, slack);
        }
        ly[y] = -minSlack;
    }

    // Remove non-tight matched edges
    for (int s = 0; s < N; s++) {
        int t = Mx[s];
        if (t == -1)
            continue;
        if (lx[s] + ly[t] != w_(s, t)) {
            Mx[s] = -1;
        } else {
            My[t] = s;
        }
    }

    // The main loop that expands the matching by one repeatedly.
    // After iteration u, all source vertexes from 0 to u are matched.
    for (int u = 0; u < N; u++) {
        if (Mx[u] != -1)
            continue;

        // Reset variables for next phase
        for (int i = 0; i < N; i++) {
            S[i] = false;
            T[i] = false;
            NlS[i] = -1;
        }
        S[u] = true;
        for (int y = 0; y < N; y++) {
            WeightType slack = lx[u] + ly[y] - w_(u, y);
            assert(slack >= 0);
            slackY[y] = slack;
            slackArgMin[y] = u;
            if (slack == 0)
                NlS[y] = u;
        }

        // Search for augmenting path
        while (true) {
            // Find a y in NlS - T
            int y = -1;
            bool yFree = false;
            for (int i = 0; i < N; i++) {
                if ((NlS[i] >= 0) && !T[i]) {
                    y = i;
                    if (My[i] == -1) {
                        yFree = true;
                        break;
                    }
                }
            }

            if (y == -1) {
                // Update labels and slackY
                WeightType al = 0;          // Initialized to silence gcc
                bool first = true;
                for (int i = 0; i < N; i++) {
                    if (T[i]) continue;
                    if (first || (slackY[i] < al)) {
                        al = slackY[i];
                        first = false;
                    }
                }
                assert(!first);
                for (int i = 0; i < N; i++) {
                    if (S[i])
                        lx[i] -= al;
                    if (T[i])
                        ly[i] += al;
                    if (!T[i]) {
                        slackY[i] -= al;
                        assert(slackY[i] >= 0);
                        if (slackY[i] == 0) {
                            NlS[i] = slackArgMin[i];
                        }
                    }
                }
            } else if (yFree) {
                // Augment M
                Mx2 = Mx;
                My2 = My;
                int t = y;
                int s = NlS[t];
                assert(s >= 0);
                Mx2[s] = t;
                My2[t] = s;
                while (s != u) {
                    t = Mx[s];
                    assert(t >= 0);
                    s = NlS[t];
                    assert(s >= 0);
                    Mx2[s] = t;
                    My2[t] = s;
                }
                Mx = Mx2;
                My = My2;
                break;
            } else {
                // Update S, T
                int z = My[y];
                S[z] = true;
                T[y] = true;

                // Update slackY, slackArgMin, NlS
                for (int i = 0; i < N; i++) {
                    WeightType slack = lx[z] + ly[i] - w_(z, i);
                    assert(slack >= 0);
                    if (slack < slackY[i]) {
                        slackY[i] = slack;
                        slackArgMin[i] = z;
                        if (slack == 0)
                            NlS[i] = z;
                    }
                }
            }
        }
    }

    // If ly becomes too large, reset it for next time, to prevent
    // arithmetic overflow.
    int maxLY = 0;
    for (int i = 0; i < N; i++) {
        maxLY = std::max(maxLY, abs(ly[i]));
    }
    if (maxLY >= std::numeric_limits<WeightType>::max() / 2)
        ly.assign(N, 0);

    return Mx;
}

template <typename WeightType>
inline void
Assignment<WeightType>::setCost(int i, int j, WeightType w) {
    w_(i, j) = -w;
}

template <typename WeightType>
inline WeightType
Assignment<WeightType>::getCost(int i, int j) const {
    return -w_(i, j);
}

template <typename WeightType>
inline int
Assignment<WeightType>::getSize() const {
    return w_.numRows();
}

#endif /* ASSIGNMENT_HPP_ */
