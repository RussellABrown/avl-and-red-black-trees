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
 *
 * MIT License
 * 
 * Copyright (c) 2017 Rene Argento
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Left-leaning red-Black tree-building functions modified from Java methods
 * of Rene Argento's implementation of answers to homework question
 * 3.3.41 on p. 455 of Robert Sedgewick and Kevin Wayne's textbook,
 * "Algorithms, Fourth Edition, Part 1" (Addison-Wesley, 2014).
 * See the following URLs that contain Rene's implementation.
 * 
 * https://github.com/reneargento/algorithms-sedgewick-wayne/blob/master/src/chapter3/section3/RedBlackBST.java
 * https://github.com/reneargento/algorithms-sedgewick-wayne/blob/master/src/chapter3/section3/Exercise41_Delete.java
 * 
 * See also the following URL that contains the RedBlackBST.java program from the textbook.
 * 
 * https://algs4.cs.princeton.edu/code/edu/princeton/cs/algs4/RedBlackBST.java.html
 *
 * To build the test executable, compile via:
 * 
 * g++ -std=c++11 -O3 llrbTree.cpp
 *
 * To disable the freed list that avoids re-use of new and delete, compile via:
 * 
 * g++ -std=c++11 -O3 -D DISABLE_FREED_LIST llrbTree.cpp
 * 
 * To enable selection of a preferred replacement node
 * when a 2-child node is deleted, compile via:
 * 
 * g++ -std=c++11 -O3 -D ENABLE_PREFERRED_TEST rbTreeIterate.cpp
 * 
 * HOWEVER, selection of a preferred replacement node
 * does not work correctly because it is not possible
 * to find the in-order predecessor of a node. The
 * problem is that a search for the maximum node
 * begins with setting color of the root node of
 * the tree to RED, or in the case of a 2-child node,
 * setting the left child of that node to RED. Upon
 * conclusion of the search, the color of the root
 * node is reset to BLACK. But this approach does
 * not work with the left child of the 2-child node
 * because, upon conclusion of the search, the color
 * of the left child affects the BLACK node count to
 * the bottom of the tree, which MUST equal the BLACK
 * node count for the right child. The root has no
 * sibling but the left child has the right child as
 * a sibling.
 *
 * For this reason, ENABLE_PREFERRED_TEST is overridden
 * by '#define FORCE_SUCCESSOR' in the erase function.
 * Hence, ENABLE_PREFERRED_TEST enables update of each
 * nodes taille field that represents the size of the
 * subtree rooted at the node but does not use that
 * size to select a preferred replacement node.
 */

#ifndef ANDERSSON_SEDGEWICK_WAYNE_ARGENTO_LLRB_TREE_H
#define ANDERSSON_SEDGEWICK_WAYNE_ARGENTO_LLRB_TREE_H

#include <iostream>
#include <exception>
#include <sstream>
#include <vector>

/*
 * The llrbTree class defines the root of the left-leaning red-black tree
 * and provides the RED and BLACK bool constants.
 */
template <typename K>
class llrbTree {

private:
    typedef bool color_t;
    static constexpr color_t RED = true;
    static constexpr color_t BLACK = false;

    /* The Node class defines a node in the LL RB tree. */
private:
    struct Node
    {
        K key;
        color_t color;
        Node *left, *right;
#ifdef ENABLE_PREFERRED_TEST
        size_t taille; /* the number of nodes in this subtree */
#endif

    public:
        Node( K const& x, bool const c ) {
            key = x;
            color = c;
            left = right = nullptr;
#ifdef ENABLE_PREFERRED_TEST
            taille = 1;
#endif
        }
    };
        
public:
    size_t nodeSize() {
        return sizeof(Node);
    }

private:
    Node *root, *freed;
    size_t count;  /* the number of nodes in the tree */
    bool a, r;     /* record modification of the tree */
  
public:
    size_t rotateL, rotateR; // rotation counters

public:
    llrbTree() {
        root = freed = nullptr;
        count = 0;
        a = r = false;
    }
    
public:
    ~llrbTree() {
        if ( root != nullptr ) {
            clear();
            clearFreed();
        }
    }

    /*
     * Delete every node in the TD RB tree.  If the tree has been
     * completely deleted via prior calls to the erase function,
     * this function will do nothing.
     * 
     * Calling parameter:
     * 
     * @param node (IN) pointer to a node
     */
private:
    void clear(Node* const node) {
        if (node == nullptr) {
            return;
        }
        clear(node->left);
        clear(node->right);
        delete node;
    }
    /*
     * Attempt to obtain a node from the freed list instead of
     * creating a new node.
     * 
     * Calling parameter:
     * 
     * @param key (IN) the key to store in the node
     */ 

public:
    void clear() {
	    clear(root);
        root = nullptr;
        count = 0;
#ifndef DISABLE_FREED_LIST
	    clearFreed();
#endif
    }

    /* Delete every node from the freed list. */
private:
    void clearFreed() {
#ifndef DISABLE_FREED_LIST
        while ( freed != nullptr ) {
            Node* next = freed->left;
            delete freed;
            freed = next;
        }
#endif
    }

    /* Report the number of nodes on the freed list. */
public:
    size_t freedSize() {
        size_t count = 0;
#ifndef DISABLE_FREED_LIST
        Node* ptr = freed;
        while(ptr != nullptr) {
            ++count;
            ptr = ptr->left;
        }
#endif
        return count;
    }

    /*
     * Prepend the specified number of nodes to the freed list.
     *
     * Calling parameters:
     *
     * @param n (IN) the number of nodes to prepend
     * @param x (IN) a dummy key argument
     */
public:
    inline void freedPreallocate( size_t const n, K const& x ) {
 #ifndef DISABLE_FREED_LIST
       for (size_t i = 0; i < n; ++i) {
            Node* p = new Node( x, RED );
            p->left = freed;
            freed = p;
       }
#endif
    }

    /*
     * Prepend the freed node to the freed list via its left pointer.
     *
     * Calling parameter:
     * 
     * q (IN) pointer to a node
     */
public:
    inline void deleteNode( Node* q ) {
#ifndef DISABLE_FREED_LIST
        q->left = freed;
        freed = q;
#else
        delete q;
#endif
    }

    /*
     * Attempt to obtain a node from the freed list instead of
     * creating a new node.
     * 
     * Calling parameter:
     * 
     * @param key (IN) the key to store in the node
     */ 
private:
    inline Node* newNode(K const& key, bool const c) {

#ifndef DISABLE_FREED_LIST
        if (freed != nullptr )
        {
            Node* ptr = freed;
            freed = freed->left;
            ptr->key = key;
            ptr->color = RED;
#ifdef ENABLE_PREFERRED_TEST
            ptr->taille = 1;
#endif
            ptr->left = ptr->right = nullptr;
            return ptr;
        } else
#endif
        {
            return new Node(key, c);
        }
    }

    /*
     * Search the tree for the existence of a key.
     *
     * Calling parameters:
     *
     * @param q (IN) pointer a node
     * @param x (IN) the key to search for
     * 
     * @return true if the key was found; otherwise, false
     */
private:
    inline bool contains( Node* const q, K const& x) {
        
        Node* p = q;            
        while ( p != nullptr ) {                    /* iterate; don't use recursion */
            if ( x < p->key ) {
                p = p->left;                        /* follow the left branch */
            } else if ( x > p->key ) {
                p = p->right;                       /* follow the right branch */
            } else {
                return true;                        /* found the key, so return true */
            }
        }
        return false;                               /* didn't find the key, so return false */
    }
    
    /*
     * Test whether a node is RED.
     *
     * Calling parameter:
     * 
     * @param p (IN) the root of the sub-tree
     *  
     * @return true is the node is RED
     */
public:
    inline bool isRed( Node* const p ) {
        if (p == nullptr) {
            return false;
        }
        return ( p->color == RED );
    }

    /*
     * If possible, rotate left about a node, which
     * is the analog of the AVL tree RR rotation.
     *
     * Calling parameter:
     * 
     * p (IN) the node
     * 
     * @return the right child of the node
     */
private:
    inline Node* rotateLeft( Node* const p ) {
        if ( p == nullptr || p->right == nullptr ) {
            return p;
        }

        ++rotateL;

        Node* p1 = p->right;

        p->right = p1->left;
        p1->left = p;

        p1->color = p->color;
        p->color = RED;

#ifdef ENABLE_PREFERRED_TEST
        p1->taille = p->taille;
        p->taille = updateSize(p);
#endif
        return p1;
    }

    /*
     * If possible, rotate right about a node, which
     * is the analog of the AVL tree LL rotation.
     *
     * Calling parameter:
     * 
     * p (IN) the node
     * 
     * @return the right child of the node
     */
private:
    inline Node* rotateRight( Node* const p ) {
        if (p == nullptr || p->left == nullptr) {
            return p;
        }

        ++rotateR;

        Node* p1 = p->left;

        p->left = p1->right;
        p1->right = p;

        p1->color = p->color;
        p->color = RED;

#ifdef ENABLE_PREFERRED_TEST
        p1->taille = p->taille;
        p->taille = updateSize(p);
#endif
        return p1;
    }

    /*
     * If possible, complement the colors of a node
     * and its children.
     * 
     * Calling parameter:
     * 
     * p (IN) the node
     */
private:
    inline void flipColors( Node* const p ) {
#if 1
        if ( p == nullptr || p->left == nullptr || p->right == nullptr ) {
            return;
        }

        // The root must have opposite color of its two children
        if ((isRed(p) && !isRed(p->left) && !isRed(p->right))
                || (!isRed(p) && isRed(p->left) && isRed(p->right))) {
            p->color = !p->color;
            p->left->color = !p->left->color;
            p->right->color = !p->right->color;
        }
#else
        if (p != nullptr) {
            p->color = !p->color;

            if (p->left != nullptr) {
                p->left->color = !p->left->color;
            }

            if (p->right != nullptr) {
                p->right->color = !p->right->color;
            }
        }
#endif
    }

    /*
     * Searches the tree recursively for a key and
     * if absent, add the key as a new node.
     * 
     * The "p" pointer is possibly modified and is returned
     * to represent the root of the sub-tree.
     *
     * Calling parameters:
     *
     * @param p (IN) pointer a node
     * @param x (IN) the key to add to the tree
     * 
     * @return the root of the rebalanced sub-tree
     */
private:
    Node* insert( Node* p, K const& x ) {
        
        if (p == nullptr) {
            a = true;
            ++count;
            return newNode( x, RED ); // Add a RED node at a leaf.
        }

        if (x < p->key) {
            p->left = insert( p->left, x );
        } else if (x > p->key) {
            p->right = insert(p->right, x );
        } else {
            // For a tree, don't insert the key twice.
            // For a map, overwrite the value.
            a = false;
        }

        if (isRed(p->right) && !isRed(p->left)) {
            p = rotateLeft(p);
        }
        if (isRed(p->left) && isRed(p->left->left)) {
            p = rotateRight(p);
        }
        if (isRed(p->left) && isRed(p->right)) {
            flipColors(p);
        }
        
#ifdef ENABLE_PREFERRED_TEST
        p->taille = updateSize(p);
#endif
        return p;
    }

private:
    inline Node* balance( Node* p ) {
        if ( p == nullptr ) {
            return nullptr;
        }

        if (isRed(p->right) && !isRed(p->left)) {
            p = rotateLeft(p);
        }

        if (isRed(p->left) && p->left != nullptr && isRed(p->left->left)) {
            p = rotateRight(p);
        }

        if (isRed(p->left) && isRed(p->right)) {
            flipColors(p);
        }

#ifdef ENABLE_PREFERRED_TEST
        p->taille = updateSize(p);
#endif
        return p;
    }

private:
    inline Node* moveRedLeft( Node* p ) {
        flipColors(p);
        if (p->right != nullptr && isRed(p->right->left)) {
            p->right = rotateRight(p->right);
            p = rotateLeft(p);
            flipColors(p);
        }
        return p;
    }

private:
    inline Node* moveRedRight( Node* p ) {
        flipColors(p);
        if ( p->left != nullptr && isRed(p->left->left) ) {
            p = rotateRight(p);
            flipColors(p);
        }
        return p;
    }

private:
    Node* min( Node* const p ) {
        if (p->left == nullptr) {
            return p;
        }
        return min(p->left);
    }

public:
    K* min() {
        return &(min(root)->key);
    }

private:
    Node* deleteMin( Node* p ) {
        if (p->left == nullptr) {
            deleteNode(p);
            r = true;
            --count;
            return nullptr;
        }

        if ( !isRed(p->left) && !isRed(p->left->left) ) {
            p = moveRedLeft(p);
        }

        p->left = deleteMin(p->left);
        return balance(p);
    }

public:
    inline bool deleteMin() {
        if ( empty() ) {
            return false;
        }

        // If both children of the root are BLACK, set the root to RED.
        if ( !isRed(root->left) && !isRed(root->right) ) {
            root->color = RED;
        }
        r = false;
        root = deleteMin(root);
        if ( !empty() ) {
            root->color = BLACK;
        }
     return r;
   }

private:
    Node* max( Node* const p ) {
        if (p->right == nullptr) {
            return p;
        }
        return max(p->right);
    }

public:
    K* max() {
        return &(max(root)->key);
    }

private:
    Node* deleteMax( Node* p ) {
        if (isRed(p->left)) {
            p = rotateRight(p);
        }

        if (p->right == nullptr) {
            deleteNode(p);
            r = true;
            --count;
            return nullptr;
        }

        if (!isRed(p->right) && !isRed(p->right->left)) {
            p = moveRedRight(p);
        }

        p->right = deleteMax(p->right);
        return balance(p);
    }

#ifdef ENABLE_PREFERRED_TEST
private:
    Node* eraseMax( Node* p ) {
        if ( p == nullptr ) {
            return nullptr;
        }

        // If both children of the node are BLACK, set the node to RED.
        if ( !isRed(p->left) && !isRed(p->right) ) {
            p->color = RED;
        }
        p = deleteMax(p);

        // This attempt to fix the red-red problem
        // doesn't work in all cases because it can
        // cause the left subtree to have a longer
        // black path than the right subtree.
        if ( p != nullptr ) {
            p->color = BLACK;
        }

        return p;
   }
#endif

public:
    inline bool deleteMax() {
        if ( empty() ) {
            return false;
        }

        // If both children of the root are BLACK, set the root to RED.
        if ( !isRed(root->left) && !isRed(root->right) ) {
            root->color = RED;
        }
        r = false;
        root = deleteMax(root);
        if ( !empty() ) {
            root->color = BLACK;
        }
        return r;
   }

    /*
     * Remove a node from the tree
     * and then rebalance the tree.
     * 
     * Calling parameters:
     * 
     * @param p (IN) pointer a node
     * @param x (IN) the key to remove from the tree
     * 
     * @return the root of the rebalanced sub-tree
     */
private:
    Node* erase( Node* p, K const& x ) {

        if ( p == nullptr ) {
            r = false;
            return nullptr;
        }

        if ( x < p->key ) {
            if (!isRed(p->left) && p->left != nullptr && !isRed(p->left->left)) {
                p = moveRedLeft(p);
            }
            p->left = erase( p->left, x );
        } else {
            if (isRed(p->left)) {
                p = rotateRight(p);
            }

            // Left child only. Symmetric treatment of right child
            // only causes the tree's size and count to mismatch.
            if ( x == p->key && p->right == nullptr) {
                deleteNode(p);
                --count;
                r = true;
                return nullptr;
            }

            if (!isRed(p->right) && p->right != nullptr && !isRed(p->right->left)) {
                p = moveRedRight(p);
            }

            if ( x == p->key ) {
                r = true;

                // Define FORCE_SUCCESSOR to override
                // ENABLE_PREFERRED_TEST so that the
                // in-order predecessor will never
                // be chosen because the eraseMax
                // function can return a red left
                // child that, in association with
                // a red parent, cannot be resolved
                // via rotations.
#define FORCE_SUCCESSOR

#ifdef ENABLE_PREFERRED_TEST
#ifdef FORCE_SUCCESSOR
		        if (false)
#else
#ifdef INVERT_PREFERRED_TEST
                if ( getSize(p->left) < getSize(p->right) )
#else
                if ( getSize(p->left) >= getSize(p->right) )
#endif
#endif
                {
                    Node* aux = max(p->left);
                    p->key = aux->key;
                    p->left = eraseMax(p->left);
                    // This attempt to fix the red-red problem
                    // doesn't work in all cases because it can
                    // cause the left subtree to have a longer
                    // black path than the right subtree.
                    if (isRed(p) && isRed(p->left)) {
                        p->left->color = BLACK;
                    }
               } else
#endif
                {
                    Node* aux = min(p->right);
                    p->key = aux->key;
                    p->right = deleteMin(p->right);
                }
            } else {
                p->right = erase( p->right, x );
            }
        }

        return balance(p);
    }

    /*
     * Check the LL RB tree for correctness, i.e.,
     * (1) no RED child of a RED parent
     * (3) no RED right child
     * (3) correct sorted order of keys
     * (4) a constant number of BLACK nodes along
     *     any path to the bottom of the tree
     * 
     * Calling parameters:
     * 
     * @param node (IN) the root of the subtree at this level of recursion
     * @param prevCount (IN) the BLACK node count from a previous call
     *                       to this checkTree function
     * @param currCount (MODIFIED) the BLACK node count at this level of recursion
     *                       that is passed to the next level of recursion;
     *                       it is incremented if this node is BLACK
     * 
     * @return the BLACK node count at this level of recursion
     */
private:
    size_t checkTree( Node* const node, size_t const prevCount, size_t currCount ) {

        // Increment the current count if the node is BLACK.
        if ( getColor(node) == BLACK ) {
            ++currCount;
        }

#ifdef ENABLE_PREFERRED_TEST
        // Check the stored size versus the computed size at this node.
        checkSize(node);
#endif

        // Check for adjacent RED nodes.
        if ( getColor(node) == RED && getColor(node->left) == RED ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " is RED and left child ";
            streamNode(node->left, buffer);
            buffer << " is also RED" << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // Check for a RED right child.
        if ( getColor(node->right) == RED ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " has RED right child ";
            streamNode(node->right, buffer);
            buffer << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // Check for correct key order.
        if ( node->left != nullptr && node->left->key >= node->key ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " left child ";
            streamNode(node->left, buffer);
            buffer << std::endl;
            throw std::runtime_error(buffer.str());
        }
        if ( node->right != nullptr && node->right->key <= node->key ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " right child ";
            streamNode(node->right, buffer);
            buffer << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // If the bottom of the tree has been reached compare counts
        // but only if the previous count is valid (i.e., non-zero).
        if ( node->left == nullptr && node->right == nullptr ) {
            if ( prevCount != 0 && currCount != prevCount ) {
                std::ostringstream buffer;
                buffer << std::endl << std::endl << "node ";
                streamNode(node, buffer);
                buffer << " current count = " << currCount
                        << "  !=  previous count = " << prevCount << std::endl;
                throw std::runtime_error(buffer.str());
            }
            return currCount;
        }
        
        // Descend to the leaves of each subtree. At least one
        // of left and right is a non-null child.
        size_t leftCount = 0, rightCount = 0;
        if ( node->left != nullptr ) {
            leftCount = checkTree( node->left, prevCount, currCount );
        }
        if ( node->right != nullptr ) {
            rightCount = checkTree( node->right, prevCount, currCount );
        }

        // If one count is zero, return the other count.
        if ( leftCount == 0 ) {
            return rightCount;
        }
        if ( rightCount == 0 ) {
            return leftCount;
        }
        // Both counts are non-zero, so compare them.
        if ( leftCount != rightCount ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " left count = " << leftCount
                   << "  !=  right count = " << rightCount << std::endl;
            throw std::runtime_error(buffer.str());
        }
        // The counts are equal, so return either of them.
        return leftCount;
    }

    /*
     * Check the LL RB tree for correctness and count
     * the number of BLACK nodes along each path to the
     * leaves and compare the previous and current counts
     * to ensure that all paths have the same count.
     * 
     * Calling parameters:
     * 
     * @param p (IN) pointer to a node
     * @param prevCount (IN) the count from the previous call
     * @param currCount (IN) the count from the prior level
     * 
     * @return the number of BLACK nodes
     */
public:
    size_t checkTree() {

        if (root == nullptr) {
            return 0;
        }

        // Check the color of the root node.
        if (getColor(root) != BLACK) {
                std::ostringstream buffer;
                buffer << std::endl << std::endl << "root ";
                streamNode(root, buffer);
                buffer << " is not black" << std::endl;
                throw std::runtime_error(buffer.str());
        }

        // Call checkTree twice:
        // (1) to initialize prevCount without comparing black counts
        // (2) to compare prevCount to the black count at each leaf 
        size_t prevCount = 0;
        prevCount = checkTree(root, prevCount, 0);
        return checkTree(root, prevCount, 0);
    }

private:
    inline void checkSize(Node* const node) {
        if (node != nullptr) {
            if ( getSize(node) != getSize(node->left) + 1 + getSize(node->right) ) {
                std::ostringstream buffer;
                buffer << std::endl << "node ";
                streamNode(node, buffer);
                buffer << " stored size = " << getSize(node)
                        << "  !=   computed size = "
                        << (getSize(node->left) + 1 + getSize(node->right)) << std::endl;
                throw std::runtime_error(buffer.str());
            }
        }
    }

private:
    void streamNode(Node* const node, std::ostringstream& buffer) {
        if (node != nullptr) {
            buffer << node->key;
            if (node->color == RED) {
                buffer << "r";
            } else {
                buffer << "b";
            }
        }
    }

private:
    void printNode( Node* const node ) {
        if (node != nullptr) {
            std::cout << node->key;
            if (node->color == RED) {
            std::cout << "r";
            } else {
                std::cout << "b";
           }
        }
    }

    /*
     * Print the keys stored in the tree, where the key of
     * the root of the tree is at the left and the keys of
     * the leaf nodes are at the right.
     * 
     * Calling parameters:
     * 
     * @param p (IN) pointer a node
     * @param d (MODIFIED) the depth in the tree
     */
private:
    void printTree( Node* const p, int d ) {
        if ( p->right != nullptr ) {
            printTree( p->right, d+1 );
        }

        for ( int i = 0; i < d; ++i ) {
            std::cout << "    ";
        }
        std::cout << p->key;
        if (p->color == RED) {
            std::cout << "r\n";
        } else {
            std::cout << "b\n";
        }

        if ( p->left != nullptr ) {
            printTree( p->left, d+1 );
        }
    }
        
    /*
     * Walk the tree in order and store each key in a vector.
     *
     * Calling parameters:
     * 
     * @param p (IN) pointer to a node
     * @param v (MODIFIED) vector of the keys
     * @param i (MODIFIED) index to the next unoccupied vector element
     */       
private:
    void getKeys( Node* const p, std::vector<K>& v, size_t& i ) {

        if ( p->left != nullptr ) {
            getKeys( p->left, v, i );
        }
        v[i++] = p->key;
        if ( p->right != nullptr ){
            getKeys( p->right, v, i );
        }
    }

    /* Return true if there are no nodes in the tree. */
public:
    bool empty() {
#ifdef ENABLE_PREFERRED_TEST
        if ( getSize(root) != count ) {
            std::ostringstream buffer;
            buffer << std::endl << "tree size = " << getSize(root) << "  !=  tree count = " << count << std::endl;
            throw std::runtime_error(buffer.str());
        }
#endif
        return ( count == 0);
    }

    /* Return the number of nodes in the tree. */
public:
    size_t size() {
#ifdef ENABLE_PREFERRED_TEST
        if ( getSize(root) != count ) {
            std::ostringstream buffer;
            buffer << std::endl << "tree size = " << getSize(root) << "  !=  tree count = " << count << std::endl;
            throw std::runtime_error(buffer.str());
        }
#endif
        return count;
    }

#ifdef ENABLE_PREFERRED_TEST
    /*
     * Return the size of the sub-tree.
     *
     * Calling parameter:
     * 
     * @param p (IN) the root of the sub-tree
     * 
     * @return the size of the subtree
     */
private:
    inline size_t getSize( Node* const p ) {
        if (p == nullptr) {
            return 0;
        }
        return p->taille;
    }

private:
    inline size_t updateSize( Node* const p ) {
        return ( getSize(p->left) + 1 + getSize(p->right) );
    }
#endif

private:
    inline bool getColor( Node* const p ) {
        if (p == nullptr) {
            return BLACK;
        }
        return p->color;
    }

    /*
     * Search the LL RB tree for the existence of a key.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to search for
     * 
     * @return true if the key was found; otherwise, false
     */
public:
    inline bool contains( K const& x ) {
        return contains( root, x );
    }
    
    /* Search the LL RB tree for a key and if absent,
     * add the key as a new node.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to add to the tree
     * 
     * @return true if the key was added as a new node; otherwise, false
     */
public:
    inline bool insert( K const& x ) {
        a = false;

        if ( root != nullptr ) {
            root = insert( root, x );
            root->color = BLACK; // Enforce a BLACK root.
        } else {
            root = newNode( x, BLACK ); // Add a BLACK node at the root;
            a = true;
            ++count;
        }
        return a;
    }

    /*
     * Remove a node from the LL RB tree and then rebalance the tree.
     * 
     * Calling parameter:
     * 
     * @param x (IN) the key to remove from the tree
     * 
     * @return true if the key was removed from the tree; otherwise, false
     */
public:
    inline bool erase( K const& x ) {
        r = false;
        if ( root != nullptr ) {
            if ( !isRed(root->left) && !isRed(root->right) ) {
                root->color = RED;
            }
            root = erase( root, x);
            if ( root != nullptr ) {
                root->color = BLACK;
            }
        }
        return r;
    }

    /*
     * Print the keys stored in the LL RB tree, where the key
     * of the root of the tree is at the left and the keys of
     * the leaf nodes are at the right.
     */
public:
    void printTree() {
        if ( root != nullptr ) {
            printTree( root, 0 );
        }
    }

    /*
     * Walks the LL RB tree in order and store each key in a vector.
     *
     * Calling parameter:
     * 
     * @param v (MODIFIED) vector of the keys
     */
public:
    void getKeys( std::vector<K>& v ) {
        if ( root != nullptr ) {
            size_t i = 0;
            getKeys( root, v, i );
        }
    }
};

#endif // ANDERSSON_SEDGEWICK_WAYNE_ARGENTO_LLRB_TREE_H
