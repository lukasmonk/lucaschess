/*
    Texel - A UCI chess engine.
    Copyright (C) 2014  Peter Österlund, peterosterlund2@gmail.com

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
 * numa.hpp
 *
 *  Created on: Jul 24, 2014
 *      Author: petero
 */

#ifndef NUMA_HPP_
#define NUMA_HPP_

#include <vector>


/** Bind search threads to suitable NUMA nodes. */
class Numa {
public:
    /** Get singleton instance. */
    static Numa& instance();

    /** Disable NUMA awareness. Useful when running several single-threaded
     *  test games simultaneously on NUMA hardware. */
    void disable();

    /** Preferred node for a given search thread. */
    int nodeForThread(int threadNo) const;

    /** Bind current thread to NUMA node determined by nodeForThread(). */
    void bindThread(int threadNo) const;

    /** Return true if threadNo runs on the same NUMA node as thread 0. */
    bool isMainNode(int threadNo) const;

private:
    Numa();

    /** Thread number to node number. */
    std::vector<int> threadToNode;

    struct NodeInfo {
        explicit NodeInfo(int n = 0, int c = 0, int t = 0);
        int node;
        int numCores;
        int numThreads;
    };
};

inline
Numa::NodeInfo::NodeInfo(int n, int c, int t)
    : node(n), numCores(c), numThreads(t) {
}

#endif /* NUMA_HPP_ */
