/*
 * Copyright (c) 2024 Russell A. Brown
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
 * std::set test program
 *
 * To build the test executable, compile via:
 * 
 * g++ -std=c++20 -O3 -o test_stdSet test_stdSet.cpp
 * 
 * To insert the keys in increasing, not random, order, compile via:
 * 
 * g++ -std=c++11 -O3 -D INSERT_INORDER -o test_stdSet test_stdSet.cpp
 * 
 * To delete the keys in increasing, not random, order, compile via:
 * 
 * g++ -std=c++11 -O3 -D DELETE_INORDER -o test_stdSet test_stdSet.cpp
 * 
 * To delete the keys in decreasing, not random, order, compile via:
 * 
 * g++ -std=c++11 -O3 -D DELETE_REVORDER -o test_burbTree test_stdSet.cpp
 * 
 * Usage:
 * 
 * test_stdSet [-k K] [-i I]
 * 
 * where the command-line options are interpreted as follows.
 * 
 * -k The number of keys to insert into the set
 * 
 * -i The number of times to iterate the test
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

int main(int argc, char **argv) {
    
    using std::cout;
    using std::endl;
    using std::ostringstream;
    using std::runtime_error;
    using std::set;
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

    // Create vectors to store the execution times for each iteration.
    vector<double> insertTime(iterations), searchTime(iterations), deleteTime(iterations);

    // Create two vectors of unique unsigned integers as large as keys.
    vector<uint32_t> insertNumbers(keys);
    for (size_t i = 0; i < keys; ++i) {
        insertNumbers[i] = i;
    }
    vector<uint32_t> deleteNumbers(insertNumbers);

    // Prepare to shuffle the vector of integers.
    std::mt19937_64 g(std::mt19937_64::default_seed);

    // Create std::set that has integer keys.
    set<uint32_t> root;

    // Build and test std::set.
    size_t treeSize;
    for (size_t it = 0; it < iterations; ++it) {

        // Shuffle the keys and insert each key into std::set.
#ifndef INSERT_INORDER
        shuffle(insertNumbers.begin(), insertNumbers.end(), g);
#endif
        auto startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < insertNumbers.size(); ++i) {
            if ( root.insert( insertNumbers[i] ).second == false) {
                ostringstream buffer;
                buffer << endl << "key " << insertNumbers[i] << " is already in tree for insert" << endl;
                throw runtime_error(buffer.str());
            }
        }
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        insertTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // No need to reshuffle the keys prior to searching std::set
        // for each key because search does not rebalance the tree and
        // hence the insertion order of the keys is irrelevant to search.
        startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < insertNumbers.size(); ++i) {
            if ( root.contains( insertNumbers[i] ) == false ) { // contains() requires C++ 20
                ostringstream buffer;
                buffer << endl << "key " << insertNumbers[i] << " is not in set for contains" << endl;
                throw runtime_error(buffer.str());
            }
        }
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        searchTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // Reshuffle the keys prior to deleting each key from std::set
        // because deletion rebalances the tree and hence the insertion
        // order of the keys may influence the performance of deletion.
#if !defined(DELETE_INORDER) && !defined(DELETE_REVORDER)
        shuffle(deleteNumbers.begin(), deleteNumbers.end(), g);
#endif
        startTime = std::chrono::steady_clock::now();
#ifndef DELETE_REVORDER
        for (size_t i = 0; i < deleteNumbers.size(); ++i)
#else
        for (int64_t i = deleteNumbers.size()-1; i >= 0; --i)
#endif
        {
            if ( root.erase( deleteNumbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << deleteNumbers[i] << " is not in set for erase" << endl;
                throw runtime_error(buffer.str());
            }
        }
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        deleteTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // Verify that the std::set is empty
        if ( root.empty() == false ) {
            ostringstream buffer;
            buffer << endl << root.size() << " keys remain in set following erasure" << endl;
            throw runtime_error(buffer.str());
        }
    }

    // Compute the means and standard deviations.
    double sumI = 0, sumI2 = 0, sumS = 0, sumS2 = 0, sumD = 0, sumD2 = 0;
    for (size_t i = 0; i < iterations; ++i) {
        sumI += insertTime[i];
        sumI2 += insertTime[i] * insertTime[i];
        sumS += searchTime[i];
        sumS2 += searchTime[i] * searchTime[i];
        sumD += deleteTime[i];
        sumD2 += deleteTime[i] * deleteTime[i];
    }
    double n = static_cast<double>(iterations);
    double insertMean = sumI / n;
    double searchMean = sumS / n;
    double deleteMean = sumD / n;
    double insertStdDev = sqrt((n * sumI2) - (sumI * sumI)) / n;
    double searchStdDev = sqrt((n * sumS2) - (sumS * sumS)) / n;
    double deleteStdDev = sqrt((n * sumD2) - (sumD * sumD)) / n;

    // Report the statistics.
    cout << endl << "number of keys in set = " << keys
         << "\titerations = " << iterations << endl << endl;
    
    cout << "insert time = " << setprecision(4) << insertMean
         << "\tstd dev = " << insertStdDev << endl;
    cout << "search time = " << setprecision(4) << searchMean
         << "\tstd dev = " << searchStdDev << endl;
    cout << "delete time = " << setprecision(4) << deleteMean
         << "\tstd dev = " << deleteStdDev << endl << endl;

    // Clear the set.
    root.clear();

    return 0;
}
