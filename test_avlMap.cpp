/*
 * Modifications Copyright (c) 2007, 2016, 2023, 2024 Russell A. Brown
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
 * AVL map-building program modified from Pascal procedures
 * 4.63 (p. 220) and 4.64 (p. 223) of Nicklaus Wirth's textbook,
 * "Algorithms + Data Structures = Programs", with correction
 * of the bug in the del procedure and replacement of del
 * by the eraseRight and eraseLeft methods. The eraseRight
 * method performs the identical operations to del, whereas
 * the eraseLeft method performs the mirror-image operations
 * to eraseRight in an attempt to improve rebalancing efficiency
 * after deletion.
 * 
 * To build the test executable, compile via: g++ -std=c++11 -O3 test_avlMap.cpp
 */

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

#include "avlMap.h"

// A basic test
int main(int argc, char **argv) {
    
    using std::cout;
    using std::endl;
    using std::ostringstream;
    using std::runtime_error;
    using std::setprecision;
    using std::shuffle;
    using std::string;
    using std::vector;

    struct timespec startTime, endTime;
    size_t iterations = 100;

    // Read the words file into a dictionary. 
    vector<string> dictionary;
    char buf[512];
    FILE *f = fopen("words.txt", "r");
    while (fgets(buf, sizeof buf, f)) {
        size_t len = strlen(buf);
        buf[len-1] = '\0';
        dictionary.push_back(string(buf));
    }
    fclose(f);

    // Create a vector of unique unsigned integers as large as the number of words.
    vector<uint32_t> numbers;
    for (size_t i = 0; i < dictionary.size(); ++i) {
        numbers.push_back(i);
    }

    // Prepare to shuffle the dictionary.
    std::mt19937_64 g(std::mt19937_64::default_seed);

    // Obtain statistics for an AVL tree that has a string key.
    avlMap<string, uint32_t> stringRoot;
    size_t stringMapSize;
    double createStringTime = 0, searchStringTime = 0, deleteStringTime = 0;
     for (size_t it = 0; it < iterations; ++it) {

         // Shuffle the dictionary and add each word to the AVL map.
        shuffle(dictionary.begin(), dictionary.end(), g);
        clock_gettime(CLOCK_REALTIME, &startTime);
        for (size_t i = 0; i < dictionary.size(); ++i) {
            if ( stringRoot.insert( dictionary[i], i ) == true ) {
                ostringstream buffer;
                buffer << endl << "key " << dictionary[i] << " is already in string tree" << endl;
                throw runtime_error(buffer.str());
            }
        }

        clock_gettime(CLOCK_REALTIME, &endTime);
        createStringTime += (endTime.tv_sec - startTime.tv_sec) +
        1.0e-9 * ((double)(endTime.tv_nsec - startTime.tv_nsec));

        // Verify that the correct number of nodes were added to the map.
        stringMapSize = stringRoot.size();
        if (stringMapSize != dictionary.size()) {
            ostringstream buffer;
            buffer << endl << "expected size for string tree = " << stringMapSize
                   << " differs from actual size = " << dictionary.size() << endl;
            throw runtime_error(buffer.str());
        }

        // Search the AVL map for each key and value.
        clock_gettime(CLOCK_REALTIME, &startTime);
        for (size_t i = 0; i < dictionary.size(); ++i) {
            if ( stringRoot.contains( dictionary[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << dictionary[i] << " is not in string tree for contains" << endl;
                throw runtime_error(buffer.str());
            }
            uint32_t const* val = stringRoot.find( dictionary[i] );
            if (val == nullptr) {
                ostringstream buffer;
                buffer << endl << "key " << dictionary[i] << " is not in string tree for find" << endl;
                throw runtime_error(buffer.str());
            } else if (*val != i) {
                ostringstream buffer;
                buffer << endl << "wrong value = " << (*val) << " for string key "
                       << dictionary[i] << " expected value = " << i << endl;
                throw runtime_error(buffer.str());
            }
        }

        clock_gettime(CLOCK_REALTIME, &endTime);
        searchStringTime += (endTime.tv_sec - startTime.tv_sec) +
        1.0e-9 * ((double)(endTime.tv_nsec - startTime.tv_nsec));

        // Shuffle the dictionary and delete each word from the AVL tree.
        shuffle(dictionary.begin(), dictionary.end(), g);
        clock_gettime(CLOCK_REALTIME, &startTime);
        for (size_t i = 0; i < dictionary.size(); ++i) {
            if ( stringRoot.erase( dictionary[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "string key " << dictionary[i] << " is not in tree for erase" << endl;
                throw runtime_error(buffer.str());
            }
        }

        clock_gettime(CLOCK_REALTIME, &endTime);
        deleteStringTime += (endTime.tv_sec - startTime.tv_sec) +
        1.0e-9 * ((double)(endTime.tv_nsec - startTime.tv_nsec));

        // Verify that the tree is empty
        if ( stringRoot.empty() == false ) {
            ostringstream buffer;
            buffer << endl << stringRoot.size() << " nodes remain in string tree following erasure" << endl;
            throw runtime_error(buffer.str());
        }
    }

    // Report the string tree statistics.
    cout << "number of words in string map = " << stringMapSize << endl;
    cout << "create string time = " << setprecision(4) << (createStringTime/(double)iterations) << " seconds" << endl;
    cout << "search string time = " << setprecision(4) << (searchStringTime/(double)iterations) << " seconds" << endl;
    cout << "delete string time = " << setprecision(4) << (deleteStringTime/(double)iterations) << " seconds" << endl;
    cout << "string insert LL = " << (stringRoot.lli/iterations) << "\tLR = " << (stringRoot.lri/iterations)
         << "\tRL = " << (stringRoot.rli/iterations) << "\tRR = " << (stringRoot.rri/iterations)
         << "\ttotal = " << ((stringRoot.lli+stringRoot.lri+stringRoot.rli+stringRoot.rri)/iterations) << endl;
    cout << "string erase  LL = " << (stringRoot.lle/iterations) << "\tLR = " << (stringRoot.lre/iterations)
         << "\tRL = " << (stringRoot.rle/iterations) << "\tRR = " << (stringRoot.rre/iterations)
         << "\ttotal = " << ((stringRoot.lle+stringRoot.lre+stringRoot.rle+stringRoot.rre)/iterations) << endl;

    // Obtain statisitics for an AVL map that has an integer key.
    avlMap<uint32_t, uint32_t> integerRoot;
    size_t integerMapSize;
    double createIntegerTime = 0, searchIntegerTime = 0, deleteIntegerTime = 0;
    for (size_t it = 0; it < iterations; ++it) {

        // Shuffle the integers and add each integer to the AVL tree.
        shuffle(numbers.begin(), numbers.end(), g);
        clock_gettime(CLOCK_REALTIME, &startTime);
        for (size_t i = 0; i < numbers.size(); i++) {
            if ( integerRoot.insert( numbers[i], i ) == true) {
                ostringstream buffer;
                buffer << endl << "key " << dictionary[i] << " is already in integer tree" << endl;
                throw runtime_error(buffer.str());
            }
        }

        clock_gettime(CLOCK_REALTIME, &endTime);
        createIntegerTime += (endTime.tv_sec - startTime.tv_sec) +
        1.0e-9 * ((double)(endTime.tv_nsec - startTime.tv_nsec));

        // Verify that the correct number of nodes were added to the tree
        integerMapSize = integerRoot.size();
        if (integerMapSize != numbers.size()) {
            ostringstream buffer;
            buffer << endl << "expected size for integer tree = " << integerMapSize
                   << " differs from actual size = " << dictionary.size() << endl;
            throw runtime_error(buffer.str());
        }

        // Search for each integer in the AVL tree.
        clock_gettime(CLOCK_REALTIME, &startTime);
        for (size_t i = 0; i < numbers.size(); i++) {
            if ( integerRoot.contains( numbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is not in integer tree for contains" << endl;
                throw runtime_error(buffer.str());
            }
            uint32_t const* val = integerRoot.find( numbers[i] );
            if (val == nullptr) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is not in integer tree for find" << endl;
                throw runtime_error(buffer.str());
            } else if (*val != i) {
                ostringstream buffer;
                buffer << endl << "wrong value = " << (*val) << " for integer key "
                       << dictionary[i] << " expected value = " << i << endl;
                throw runtime_error(buffer.str());
            }
        }

        clock_gettime(CLOCK_REALTIME, &endTime);
        searchIntegerTime += (endTime.tv_sec - startTime.tv_sec) +
        1.0e-9 * ((double)(endTime.tv_nsec - startTime.tv_nsec));

        // Shuffle the integers and delete each integer from the AVL tree.
        shuffle(numbers.begin(), numbers.end(), g);
        clock_gettime(CLOCK_REALTIME, &startTime);
        for (size_t i = 0; i < numbers.size(); i++) {
            if ( integerRoot.erase( numbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "integer key " << numbers[i] << " is not in tree for erase" << endl;
                throw runtime_error(buffer.str());
            }
        }

        clock_gettime(CLOCK_REALTIME, &endTime);
        deleteIntegerTime += (endTime.tv_sec - startTime.tv_sec) +
        1.0e-9 * ((double)(endTime.tv_nsec - startTime.tv_nsec));

        // Verify that the tree is empty
        if ( integerRoot.empty() == false ) {
            ostringstream buffer;
            buffer << endl << integerRoot.size() << " nodes remain in integer tree following erasure" << endl;
            throw runtime_error(buffer.str());
        }
    }

    // Report the integer statistics.
    cout << "number of words in integer map = " << integerMapSize << endl;
    cout << "create integer time = " << setprecision(4) << (createIntegerTime/(double)iterations) << " seconds" << endl;
    cout << "search integer time = " << setprecision(4) << (searchIntegerTime/(double)iterations) << " seconds" << endl;
    cout << "delete integer time = " << setprecision(4) << (deleteIntegerTime/(double)iterations) << " seconds" << endl;
    cout << "integer insert LL = " << (integerRoot.lli/iterations) << "\tLR = " << (integerRoot.lri/iterations)
         << "\tRL = " << (integerRoot.rli/iterations) << "\tRR = " << (integerRoot.rri/iterations)
         << "\ttotal = " << ((integerRoot.lli+integerRoot.lri+integerRoot.rli+integerRoot.rri)/iterations) << endl;
    cout << "integer erase  LL = " << (integerRoot.lle/iterations) << "\tLR = " << (integerRoot.lre/iterations)
         << "\tRL = " << (integerRoot.rle/iterations) << "\tRR = " << (integerRoot.rre/iterations)
         << "\ttotal = " << ((integerRoot.lle+integerRoot.lre+integerRoot.rle+integerRoot.rre)/iterations) << endl;

    return 0;
}
