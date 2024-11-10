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
 * AVL tree test program
 * To build the test executable, compile via:
 * 
 * g++ -std=c++11 -O3 -o testAVLTree.out testAVLTree.cpp
 * 
 * The avlTree.h file describes compilation options.
 */

#include "avlTree.h"

int main() {

    using std::cin;
    using std::cout;
    using std::endl;
    using std::ostringstream;
    using std::runtime_error;
    using std::vector;

   /* 22 keys, one of which (14) is duplicated */
    
   vector<int> const keys{ 8, 9, 11, 15, 19, 20, 21, 7, 3, 2, 1, 5, 6, 4, 13, 14, 10, 12, 14, 17, 16, 18 };

   /* Present and missing keys */

    int const presentKey = 13, duplicatekey = 14, missingKey = 0;
    
    char ch;
    avlTree<int> t;

    /* Add each key to the AVL tree. */

    for ( size_t i = 0; i < keys.size(); ++i ) {
        cout << endl << "press return to add " << keys[i] << endl;
        ch = cin.get();
        if ( t.insert(keys[i]) == false && keys[i] != duplicatekey ) {
            cout << "error: failure to insert key " << keys[i] << endl;
        }
        cout << "tree contains " << t.size() << " nodes" << endl << endl;
        t.printTree();
        t.checkTree();
    }

    cout << endl << "*** balanced tree completed; ordered keys follow ***" << endl << endl;

    /*
     * Retrieve the keys sorted in ascending order and store them in a vector.
     * Pre-allocate the vector to avoid re-sizing it.
     */

    vector<int> sortedKeys( t.size() );
    t.getKeys( sortedKeys );
    for (size_t i = 0; i < sortedKeys.size(); ++i) {
        cout << sortedKeys[i] << " ";
    }
    cout << endl;

    /* Test the contains function. */

    if ( t.contains( presentKey ) == false ) {
        cout << endl << "error: does not contain key " << presentKey << endl;
    }
    if ( t.contains( missingKey ) == true ) {
        cout << endl << "error: contains missing key " << missingKey << endl;
    }

    /* Test the erase function for a missing key. */

    if ( t.erase( missingKey ) == true ) {
        cout << endl << "error: erased missing key " << missingKey << endl;
    }

    /* Delete each key from the AVL tree. */

    for ( size_t i = 0; i < keys.size(); ++i ) {
        cout << endl << "press return to remove " << keys[i] << endl;
        ch = cin.get();
        if ( t.erase(keys[i]) == false && keys[i] != duplicatekey ) {
            cout << "error: failure to erase key " << keys[i] << endl;
        }
        cout << "tree contains " << t.size() << " nodes" << endl << endl;
        t.printTree();
        t.checkTree();
    }
    cout << "all done" << endl << endl;
    
    return 0;
}