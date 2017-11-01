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
#include <map>


/** Bind search threads to suitable NUMA nodes. */
class Numa {
public:
    /** Get singleton instance. */
    static Numa& instance();

    /** Get number of NUMA nodes, cores and threads. For a non-NUMA system,
     *  the number of NUMA nodes is 1. */
    void getConcurrency(int& nodes, int& cores, int& threads);

    /** Disable NUMA awareness. Useful when running several single-threaded
     *  test games simultaneously on NUMA hardware. */
    void disable();

    /** Bind current thread to NUMA node determined by nodeForThread(). */
    void bindThread(int threadNo) const;

    /** Return true if threadNo runs on the same NUMA node as thread 0. */
    bool isMainNode(int threadNo) const;

private:
    Numa();

    /** Preferred node for a given search thread. */
    int nodeForThread(int threadNo) const;

    struct NodeInfo {
        int node = 0;
        int numCores = 0;
        int numThreads = 0;
    };
    /** Get information about all NUMA nodes. Not used for WIN32. */
    void getNodeInfo(std::vector<NodeInfo>& nodes);

    /** Get information about all NUMA nodes. Not used for WIN32.
     *  On non-NUMA hardware, node -1 is used. */
    void getNodeInfoMap(int maxNode, std::map<int, NodeInfo>& nodeInfo);

    /** Thread number to node number. */
    std::vector<int> threadToNode;
};

#endif /* NUMA_HPP_ */
