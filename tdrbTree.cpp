/*
 * Modifications Copyright (c) 2024 Russell A. Brown
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Top-down red-black tree test program
 *
 * To build the test executable, compile via:
 * 
 * g++ -std=c++11 -O3 -o tdrbTree.out tdrbTree.cpp
 * 
 * The tdrbTree.h file describes compilation options.
 * 
 * Usage:
 * 
 * tdrbTree.out [-k K] [-i I]
 * 
 * where the command-line options are interpreted as follows.
 * 
 * -k The number of keys to insert into the TDRB tree
 * 
 * -i The number of times to iterate the test
 */

#include "tdrbTree.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

int main(int argc, char **argv) {
    
    using std::cout;
    using std::endl;
    using std::ostringstream;
    using std::runtime_error;
    using std::setprecision;
    using std::shuffle;
    using std::string;
    using std::vector;

    int iterations = 1;
    int keys = 4194304;

    // Parse the command-line arguments.
    for (size_t i = 1; i < argc; ++i) {
        if (0 == strcmp(argv[i], "-k") || 0 == strcmp(argv[i], "--keys")) {
            keys = atol(argv[++i]);
            if (keys <= 0) {
                ostringstream buffer;
                buffer << "\n\nnodes = " << keys << "  <= 0" << endl;
                throw runtime_error(buffer.str());
            }
            continue;
        }
        if (0 == strcmp(argv[i], "-i") || 0 == strcmp(argv[i], "--iterations")) {
            iterations = atol(argv[++i]);
            if (iterations <= 0) {
                ostringstream buffer;
                buffer << "\n\niterations = " << iterations << "  <= 0" << endl;
                throw runtime_error(buffer.str());
            }
            continue;
        }
        {
            ostringstream buffer;
            buffer << "\n\nillegal command-line argument: " << argv[i] << endl;
            throw runtime_error(buffer.str());
        }
    }

    // Create vectors to store the execution times and rotations for each iteration.
    vector<double> insertTime(iterations), searchTime(iterations), deleteTime(iterations);
    vector<size_t> sri(iterations), dri(iterations), sre(iterations), dre(iterations);

    // Create a vector of unique unsigned integers as large as keys.
    vector<uint32_t> numbers(keys);
    for (size_t i = 0; i < keys; ++i) {
        numbers[i] = i;
    }

    // Prepare to shuffle the vector of integers.
    std::mt19937_64 g(std::mt19937_64::default_seed);

    // Create an RB tree that has integer keys and preallocate its freed list.
    tdrbTree<uint32_t> root;
    root.freedPreallocate( keys, 0 );
#ifndef DISABLE_FREED_LIST
    if ( root.freedSize() != keys ) {
        ostringstream buffer;
        buffer << endl << "freed list size following pre-allocate = " << root.freedSize()
               << "  != number of keys = " << keys << endl;
        throw runtime_error(buffer.str());
    }
#endif

    // Build and test the TDRB tree.
    size_t treeSize;
    for (size_t it = 0; it < iterations; ++it) {

        // Reset the rotation counters to 0.
        root.singleRotationCount = root.doubleRotationCount = 0;

        // Shuffle the keys and add each key to the TDRB tree.
        shuffle(numbers.begin(), numbers.end(), g);
        auto startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < numbers.size(); ++i) { 
            if ( root.insert( numbers[i] ) == false) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is already in tree for insert" << endl;
                throw runtime_error(buffer.str());
            }
        }
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        insertTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // Record rotation counts for insertion.
        sri[it] = root.singleRotationCount;
        dri[it] = root.doubleRotationCount;

        // Verify that the correct number of nodes were added to the TDRB tree.
        treeSize = root.size();
        if (treeSize != numbers.size()) {
            ostringstream buffer;
            buffer << endl << "expected size for tree = " << treeSize
                   << " differs from actual size = " << numbers.size() << endl;
            throw runtime_error(buffer.str());
        }

        // Verify that each path to the bottom of the string tree has an equal
        // number of BLACK nodes and check the validity of the tree.
        root.checkTree();

        // No need to reshuffle the keys prior to searching the TDRB tree
        // for each key because search does not rebalance the tree and
        // hence the insertion order of the keys is irrelevant to search.
        startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < numbers.size(); ++i) {
            if ( root.contains( numbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is not in tree for contains" << endl;
                throw runtime_error(buffer.str());
            }
        }
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        searchTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // Reset the rotation counters to 0.
        root.singleRotationCount = root.doubleRotationCount = 0;

        // Reshuffle the keys prior to deleting each key from the TDRB tree
        // because deletion rebalances the tree and hence the insertion
        // order of the keys may influence the performance of deletion.
        shuffle(numbers.begin(), numbers.end(), g);
        startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < numbers.size(); i++) {
            if ( root.erase( numbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is not in tree for erase" << endl;
                throw runtime_error(buffer.str());
            }
        }
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        deleteTime[it] = static_cast<double>(duration.count()) / 1000000.;

       // Record rotation counts for deletion.
        sre[it] = root.singleRotationCount;
        dre[it] = root.doubleRotationCount;

        // Verify that the TDRB tree is empty.
        if ( root.empty() == false ) {
            ostringstream buffer;
            buffer << endl << root.size() << " nodes remain in tree following erasure" << endl;
            throw runtime_error(buffer.str());
        }

        // Check the size of the freed list.
#ifndef DISABLE_FREED_LIST
        if ( root.freedSize() != keys ) {
            ostringstream buffer;
            buffer << endl << "freed list size following erasure = " << root.freedSize()
                << "  != number of keys = " << keys << endl;
            throw runtime_error(buffer.str());
        }
#endif
    }

    // Compute the means and standard deviations.
    double sumI = 0, sumI2 = 0, sumS = 0, sumS2 = 0, sumD = 0, sumD2 = 0;
    double sumsri = 0, sumsri2 = 0, sumdri = 0, sumdri2 = 0, sumri = 0, sumri2 = 0;
    double sumsre = 0, sumsre2 = 0, sumdre = 0, sumdre2 = 0, sumre = 0, sumre2 = 0;
    for (size_t i = 0; i < iterations; ++i) {
        sumI += insertTime[i];
        sumI2 += insertTime[i] * insertTime[i];
        sumS += searchTime[i];
        sumS2 += searchTime[i] * searchTime[i];
        sumD += deleteTime[i];
        sumD2 += deleteTime[i] * deleteTime[i];
        sumsri += sri[i] - 2*dri[i];  // a double rotation also counts 2 single rotations
        sumsri2 += (sri[i] - 2*dri[i]) * (sri[i] - 2*dri[i]);
        sumdri += dri[i];
        sumdri2 += dri[i] * dri[i];
        sumri += sri[i];
        sumri2 += sri[i] * sri[i];
        sumsre += sre[i] - 2*dre[i];  // a double rotation also counts 2 single rotations
        sumsre2 += (sre[i] - 2*dre[i]) * (sre[i] - 2*dre[i]);
        sumdre += dre[i];
        sumdre2 += dre[i] * dre[i];
        sumre += sre[i];
        sumre2 += sre[i] * sre[i];
    }
    double n = static_cast<double>(iterations);
    double insertMean = sumI / n;
    double searchMean = sumS / n;
    double deleteMean = sumD / n;
    double sriMean = sumsri / n;
    double driMean = sumdri / n;
    double sreMean = sumsre / n;
    double dreMean = sumdre / n;
    double riMean = sumri / n;
    double reMean = sumre / n;

    double insertStdDev = sqrt((n * sumI2) - (sumI * sumI)) / n;
    double searchStdDev = sqrt((n * sumS2) - (sumS * sumS)) / n;
    double deleteStdDev = sqrt((n * sumD2) - (sumD * sumD)) / n;
    double sriStdDev = sqrt((n * sumsri2) - (sumsri * sumsri)) / n;
    double driStdDev = sqrt((n * sumdri2) - (sumdri * sumdri)) / n;
    double riStdDev = sqrt((n * sumri2) - (sumri * sumri)) / n;
    double sreStdDev = sqrt((n * sumsre2) - (sumsre * sumsre)) / n;
    double dreStdDev = sqrt((n * sumdre2) - (sumdre * sumdre)) / n;
    double reStdDev = sqrt((n * sumre2) - (sumre * sumre)) / n;

    // Report the statistics.
    cout << endl << "node size = " << root.nodeSize()
         << " bytes\tnumber of keys in tree = " << treeSize
         << "\titerations = " << iterations << endl << endl;
    
    cout << "insert time = " << setprecision(4) << insertMean
         << "\tstd dev = " << insertStdDev << endl;
    cout << "search time = " << setprecision(4) << searchMean
         << "\tstd dev = " << searchStdDev << endl;
    cout << "delete time = " << setprecision(4) << deleteMean
         << "\tstd dev = " << deleteStdDev << endl << endl;
    
    cout << "insert single rotate = " << static_cast<size_t>(sriMean)
         << "\tstd dev = " << static_cast<size_t>(sriStdDev)
         << "\tdouble rotate = " << static_cast<size_t>(driMean)
         << "\tstd dev = " << static_cast<size_t>(driStdDev)
         << "\ttotal rotate = " << static_cast<size_t>(riMean)
         << "\tstd dev = " << static_cast<size_t>(riStdDev) << endl;
    cout << "delete single rotate = " << static_cast<size_t>(sreMean)
         << "\tstd dev = " << static_cast<size_t>(sreStdDev)
         << "\tdouble rotate = " << static_cast<size_t>(dreMean)
         << "\tstd dev = " << static_cast<size_t>(dreStdDev)
         << "\ttotal rotate = " << static_cast<size_t>(reMean)
         << "\tstd dev = " << static_cast<size_t>(reStdDev) << endl << endl;
    // Clear the TDRB tree.
    root.clear();

    return 0;
}
