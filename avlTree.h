/*
 * Modifications Copyright (c) 1996, 2016, 2023, 2024 Russell A. Brown
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
 * AVL tree-building functions modified from Pascal procedures
 * 4.63 (p. 220) and 4.64 (p. 223) of Nicklaus Wirth's textbook,
 * "Algorithms + Data Structures = Programs" with correction
 * of the bug in the del procedure and bifurcation of that
 * procedure into the eraseLeft and eraseRight functions.  The
 * eraseRight function performs the identical operations to del,
 * whereas the eraseLeft function performs the mirror-image
 * operations to del. Selection between these two functions
 * reduces the number of rotations required following deletion.
 *
 * Compile with a test program, for example, test_avlTree.cpp via:
 * 
 * g++ -std=c++11 -O3 test_avlTree.cpp
 * 
 * To enable parent pointers, compile via:
 * 
 * g++ -std=c++11 -O3 -D PARENT test_avlTree.cpp
 * 
 * To enable selection of a preferred replacment node
 * when a 2-child node is deleted, compile via:
 * 
 * g++ -std=c++11 -O3 -D ENABLE_PREFERRED_TEST test_avlTree.cpp
 * 
 * To invert selection of a preferred replacement node
 * when the balance of the 2-child node is 0, compile via:
 * 
 * g++ -std=c++11 -O3 -D ENABLE_PREFERRED_TEST -D INVERT_PREFERRED_TEST test_avlTree.cpp
 *
 * To disable the freed list that avoids re-use of new and delete, compile via:
 * 
 * g++ -std=c++11 -O3 -D DISABLE_FREED_LIST test_avlTree.cpp
 * 
 * To preallocate the freed list as a vector of avl tree nodes, compile via:
 * 
 * g++ -std=c++11 -O3 -D PREALLOCATE test_avlTree.cpp
 */

#ifndef ADELSON_VELSKII_LANDIS_WIRTH_AVL_TREE_RECURSE_H
#define ADELSON_VELSKII_LANDIS_WIRTH_AVL_TREE_RECURSE_H

#include <iostream>
#include <exception>
#include <sstream>
#include <vector>

/*
 * The avlTree class defines the root of the AVL tree and stores the
 * lli, lri, rli, rri, lle, lre, rle, and rre rotation counters
 * and the h, a, and r boolean variables.
 */
template <typename K>
class avlTree
{
private:
    typedef int8_t bal_t;

private:
    struct Node {
        K key;          // the key stored in this node
        bal_t bal;      // the left/right balance that assumes values of -1, 0, or +1
        Node *left, *right;
#ifdef PARENT
        Node* parent ;
#endif
       
        Node( K const& x, bool& h ) {
            h = true;  // the height has changed
            bal = 0;   // the subtree is balanced at this node
            key = x;
            left = right = nullptr;
#ifdef PARENT
            parent = nullptr;
# endif
       }

       Node() {
            bal = 0;   // the subtree is balanced at this node
            left = right = nullptr;
#ifdef PARENT
            parent = nullptr;
# endif
   }
};

public:
    size_t nodeSize() {
        return sizeof(Node);
    }

private:
    Node* root;     // the root of the tree
    size_t count;   // the number of nodes in the tree
    bool h, a, r;   // record modification of the tree

#ifndef DISABLE_FREED_LIST
    Node* freed;    // the freed list
#ifdef PREALLOCATE
    std::vector<Node> nodes;
#endif
#endif
  
public:
    size_t lle, lre, rle, rre, lli, lri, rli, rri;  // the rotation counters
   
public:
    avlTree() {
        root = nullptr;
        lle = lre = rle = rre = lli = lri = rli = rri = count = 0;
        h = a = r = false;
#ifndef DISABLE_FREED_LIST
        freed = nullptr;
#endif
    }
    
public:
    ~avlTree() {
        clear();
    }

    /*
     * Delete every node in the AVL tree.  If the tree has been
     * completely deleted via prior calls to the erase function,
     * this function will do nothing.
     * 
     * Calling parameter:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     */
public:
    void clear(Node* const p) {
        if (p == nullptr) {
            return;
        }
        clear(p->left);
        clear(p->right);
        delete p;
    }
    
    /* Delete every node in the AVL tree and on the freed list. */
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
#ifndef PREALLOCATE
        while ( freed != nullptr ) {
            Node* next = freed->left;
            delete freed;
            freed = next;
        }
#else
        nodes.clear();
#endif
        freed = nullptr;
#endif
    }

    /*
     * Attempt to obtain a node from the freed list instead of
     * creating a new node.
     * 
     * Calling parameters:
     * 
     * @param x (IN) the key to store in the node
     * @param h (MODIFIED) specifies that the tree height has changed
     * 
     * @return a pointer to the node
     */ 
private:
    inline Node* newNode( K const& x, bool& h ) {

#ifndef DISABLE_FREED_LIST
        if (freed != nullptr )
         {
            Node* p = freed;
            freed = freed->left;
            h = true;    // the height has changed
            p->bal = 0;  // the subtree is balanced at this node
            p->key = x;
            p->left = p->right = nullptr;
#ifdef PARENT
            p->parent = nullptr;
#endif
            return p;
        } else
#endif
        {
            return new Node( x, h );
        }
    }

    /*
     * Prepend a node to the freed list instead of deleting it.
     *
     * Calling parameter:
     *
     * @param q (IN) pointer to the node
     */
private:
    inline void deleteNode( Node* q ) {
#ifndef DISABLE_FREED_LIST
        q->left = freed;
        freed = q;
#else
        delete q;
#endif
    }
    
    /* Report the number of nodes on the freed list. */
public:
    size_t freedSize() {
        size_t count = 0;
 #ifndef DISABLE_FREED_LIST
       Node* p = freed;
        while(p != nullptr) {
            ++count;
            p = p->left;
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
     */
public:
    void freedPreallocate( size_t const n ) {
#ifndef DISABLE_FREED_LIST
#ifndef PREALLOCATE
       for (size_t i = 0; i < n; ++i) {
            Node* p = new Node();
            p->left = freed;
            freed = p;
        }
#else
        nodes.resize(n);
        for (size_t i = 0; i < n; ++i) {
            Node* p = &nodes[i];
            p->left = freed;
            freed = p;
        }
#endif
#endif
    }

    /* Return the number of nodes in the AVL tree. */
public:
    size_t size() {
        return count;
    }

    /* Return true if there are no nodes in the AVL tree. */
public:
    bool empty() {
        return ( count == 0 );
    }

    /*
     * Search the tree for the existence of a key.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to search for
     * 
     * @return true if the key was found; otherwise, false
     */
public:
    inline bool contains( K const& x ) {
        return contains(root, x);
    }
    
   /* Search the tree for the existence of a key.
    *
    * Calling parameters:
    *
    * @param p (IN) the root of the subtree at this level of recursion
    * @param x (IN) the key to search for
    * 
    * @return true if the key was found; otherwise, false
    */
private:
    inline bool contains( Node* const p, K const& x) {

        Node* q = p;
        while ( q != nullptr ) {                    // iterate; don't use recursion
            if ( x < q->key ) {
                q = q->left;                        // follow the left branch
            } else if ( x > q->key ) {
                q = q->right;                       // follow the right branch
            } else {
                return true;                        // found the key, so return true
            }
        }
        return false;                               // didn't find the key, so return false
    }
    
    /*
     * Search the tree for the existence of a key.
     * If the key is not found, it is is added to
     * the tree as a new node. If the key is found,
     * it is ignored. Then the tree is rebalanced
     * if necessary.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to add to the tree
     * 
     * @return true if the key was added as a new node; otherwise, false
     */
public:
#ifdef PARENT
    inline bool insert( K const& x ) {
        h = false, a = true;
        if ( root != nullptr ) {
            root = insert( nullptr, root, x );
            if ( a == true ) {
                ++count;
            }
        } else {
            root = newNode( x, h );
            root->parent = nullptr;
            ++count;
        }
        return a;
    }
#else
    inline bool insert( K const& x ) {
        h = false, a = true;
        if ( root != nullptr ) {
            root = insert( root, x );
            if ( a == true ) {
                ++count;
            }
        } else {
            root = newNode( x, h );
            ++count;
        }
        return a;
    }
#endif

    /*
     * Removes a node from the tree. Then the tree
     * is rebalanced if necessary.
     * 
     * Calling parameter:
     * 
     * @param x (IN) the key to remove from the tree
     * 
     * @return true if the key was removed from the tree; otherwise, false
     */
public:
    inline bool erase( K const& x ) {
        h = false, r = false;
        if ( root != nullptr ) {
            root = erase( root, x );
            if ( r == true ) {
                --count;
            }
        }
        return r;
    }

    /*
     * Rebalance following insertion of a left node.
     * 
     * Calling parameter:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * 
     * @return the root of the rebalanced subtree
     */
private:
    inline Node* balanceInsertLeft( Node* p ) {
        switch ( p->bal ) {
            case 1:                         // balance restored
                p->bal = 0;
                h = false;
                break;
            case 0:                         // tree has become more unbalanced
                p->bal = -1;
                break;
            case -1:		                // tree must be rebalanced
                Node* p1 = p->left;
                if ( p1->bal == -1 ) {		// single LL rotation
                    lli++;
                    p->left = p1->right;
#ifdef PARENT
                    if ( p->left != nullptr ) {
                        p->left->parent = p;
                    }
                    p1->parent = p->parent;
                    p->parent = p1;
#endif
                    p1->right = p;
                    p->bal = 0;
                    p = p1;
                } else {			        // double LR rotation
                    lri++;
                    Node* p2 = p1->right;
                    p1->right = p2->left;
#ifdef PARENT
                    if ( p1->right != nullptr ) {
                        p1->right->parent = p1;
                    }
#endif
                    p2->left = p1;
                    p->left = p2->right;
#ifdef PARENT
                    if ( p->left != nullptr ) {
                        p->left->parent = p;
                    }
                    p->parent = p1->parent = p2;
                    p2->parent = p->parent;
#endif
                    p2->right = p;
                    if ( p2->bal == -1 ) {
                        p->bal = 1;
                    } else {
                        p->bal = 0;
                    }
                    if ( p2->bal == 1 ) {
                        p1->bal = -1;
                    } else {
                        p1->bal = 0;
                    }
                    p = p2;
                }
                p->bal = 0;
                h = false;
                break;
        }
        return p;
    }

    /*
     * Rebalance following insertion of a right node.
     * 
     * Calling parameter:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * 
     * @return the root of the rebalanced subtree
     */
private:
    inline Node* balanceInsertRight( Node* p ) {
        switch ( p->bal ) {
            case -1:                        // balance restored
                p->bal = 0;
                h = false;
                break;
            case 0:                         // tree has become more unbalanced
                p->bal = 1;
                break;
            case 1:                         // tree must be rebalanced
                Node* p1 = p->right;
                if ( p1->bal == 1 ) {       // single RR rotation
                    rri++;
                    p->right = p1->left;
#ifdef PARENT
                    if ( p->right != nullptr ) {
                        p->right->parent = p;
                    }
                    p1->parent = p->parent;
                    p->parent = p1;
#endif
                    p1->left = p;
                    p->bal = 0;
                    p = p1;
                } else {                    // double RL rotation
                    rli++;
                    Node* p2 = p1->left;
                    p1->left = p2->right;
#ifdef PARENT
                    if ( p1->left != nullptr ) {
                        p1->left->parent = p1;
                    }
#endif
                    p2->right = p1;
                    p->right = p2->left;
#ifdef PARENT
                    if ( p->right != nullptr ) {
                        p->right->parent = p;
                    }
                    p->parent = p1->parent = p2;
                    p2->parent = p->parent;
#endif
                    p2->left = p;
                    if ( p2->bal == 1 ) {
                        p->bal = -1;
                    } else {
                        p->bal = 0;
                    }
                    if ( p2->bal == -1 ) {
                        p1->bal = 1;
                    } else {
                        p1->bal = 0;
                    }
                    p = p2;
                }
                p->bal = 0;
                h = false;
                break;
        }
        return p;
    }

    /*
     * Search the tree for the existence of a key.
     * If the key is not found, it is is added to
     * the tree as a new node. If the key is found,
     * it is ignored. Then the tree is rebalanced
     * if necessary.
     *
     * Calling parameter:
     *
     * @param q (IN) pointer to parent of the root of the subtree
     * @param p (IN) the root of the subtree at this level of recursion
     * @param x (IN) the key to add to the tree
     * 
     * @return the root of the rebalanced subtree
     */
public:
#ifdef PARENT
    Node* insert( Node* q, Node* p,  K const& x ) {
        
        if ( x < p->key ) {                         // search the left branch?
            if ( p->left != nullptr ) {
                p->left = insert( p, p->left, x );
            } else {
                Node* r = newNode( x, h );
                r->parent = q;
                p->left = r;
                a = true;
            }
            if ( h ) {                              // left branch has grown higher
                p = balanceInsertLeft( p );
            }
        } else if ( x > p->key ) {                  // search the right branch?
            if ( p->right != nullptr ) {
                p->right = insert( p, p->right, x );
            } else {
                Node*r = newNode( x, h );
                r->parent = q;
                p->right = r;
                a = true;
            }
            if ( h ) {                              // right branch has grown higher
                p = balanceInsertRight( p );
            }
        } else {
            // The key is already in the tree.
            // For a tree, don't insert the key twice.
            // For a map, overwrite the value.
            h = false;
            a = false;
        }

        return p;  // the root of the rebalanced subtree
    }
#else
    Node* insert( Node* p,  K const& x ) {
        
        if ( x < p->key ) {                         // search the left branch?
            if ( p->left != nullptr ) {
                p->left = insert( p->left, x );
            } else {
                p->left = newNode( x, h );
                a = true;
            }
            if ( h ) {                              // left branch has grown higher
                p = balanceInsertLeft( p );
            }
        } else if ( x > p->key ) {                  // search the right branch?
            if ( p->right != nullptr ) {
                p->right = insert( p->right, x );
            } else {
                p->right = newNode( x, h );
                a = true;
            }
            if ( h ) {                              // right branch has grown higher
                p = balanceInsertRight( p );
            }
        } else {
            // The key is already in the tree.
            // For a tree, don't insert the key twice.
            // For a map, overwrite the value.
            h = false;
            a = false;
        }

        return p;  // the root of the rebalanced subtree
    }
#endif
    
    /*
     * Rebalance following deletion of a left node.
     * 
     * Calling parameter:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * 
     * @return the root of the rebalanced subtree
     */
private:
    inline Node* balanceEraseLeft( Node* p ) {
        
        switch ( p->bal ) {
            case -1:                    // balance restored
                p->bal = 0;
                break;
            case 0:                     // tree has become more unbalanced
                p->bal = 1;
                h = false;
                break;
            case 1:                     // tree must be rebalanced
                Node* p1 = p->right;
                if ( p1->bal >= 0 ) {   // single RR rotation
                    rre++;
                    p->right = p1->left;
#ifdef PARENT
                    if ( p->right != nullptr ) {
                        p->right->parent = p;
                    }
                    p1->parent = p->parent;
                    p->parent = p1;
#endif
                    p1->left = p;
                    if ( p1->bal == 0 ) {
                        p->bal = 1;
                        p1->bal = -1;
                        h = false;
                    } else {
                        p->bal = 0;
                        p1->bal = 0;
                    }
                    p = p1;
                } else {				  // double RL rotation
                    rle++;
                    Node* p2 = p1->left;
                    p1->left = p2->right;
#ifdef PARENT
                    if ( p1->left != nullptr ) {
                        p1->left->parent = p1;
                    }
#endif
                    p2->right = p1;
                    p->right = p2->left;
#ifdef PARENT
                    if ( p->right != nullptr ) {
                        p->right->parent = p;
                    }
                    p->parent = p1->parent = p2;
                    p2->parent = p->parent;
#endif
                    p2->left = p;
                    if ( p2->bal == 1 ) {
                        p->bal = -1;
                    } else {
                        p->bal = 0;
                    }
                    if ( p2->bal == -1 ) {
                        p1->bal = 1;
                    } else {
                        p1->bal = 0;
                    }
                    p = p2;
                    p->bal = 0;
                }
                break;
        }
        return p; // the root of the rebalanced subtree
    }
    
    /*
     * Rebalances following deletion of a right node.
     * 
     * Calling parameter:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * 
     * @return the root of the rebalanced subtree
     */
private:
    inline Node* balanceEraseRight( Node* p ) {
        
        switch ( p->bal ) {
            case 1:                     // balance restored
                p->bal = 0;
                break;
            case 0:                     // tree has become more unbalanced
                p->bal = -1;
                h = false;
                break;
            case -1:                    // tree must be rebalanced
                Node* p1 = p->left;
                if ( p1->bal <= 0 ) {   // single LL rotation
                    lle++;
                    p->left = p1->right;
#ifdef PARENT
                    if ( p->left != nullptr ) {
                        p->left->parent = p;
                    }
                    p1->parent = p->parent;
                    p->parent = p1;
#endif
                    p1->right = p;
                    if ( p1->bal == 0 ) {
                        p->bal = -1;
                        p1->bal = 1;
                        h = false;
                    } else {
                        p->bal = 0;
                        p1->bal = 0;
                    }
                    p = p1;
                } else {				  // double LR rotation
                    lre++;
                    Node* p2 = p1->right;
                    p1->right = p2->left;
#ifdef PARENT
                    if ( p1->right != nullptr ) {
                        p1->right->parent = p1;
                    }
#endif
                    p2->left = p1;
                    p->left = p2->right;
#ifdef PARENT
                    if ( p->left != nullptr ) {
                        p->left->parent = p;
                    }
                    p->parent = p1->parent = p2;
                    p2->parent = p->parent;
#endif
                    p2->right = p;
                    if ( p2->bal == -1 ) {
                        p->bal = 1;
                    } else {
                        p->bal = 0;
                    }
                    if ( p2->bal == 1 ) {
                        p1->bal = -1;
                    } else {
                        p1->bal = 0;
                    }
                    p = p2;
                    p->bal = 0;
                }
                break;
        }
        return p;  // the root of the rebalanced subtree
    }
    
    /*
     * Replace the node to be deleted with the leftmost node of
     * the right subtree after copying the key from that leftmost
     * node to the key of the node to be deleted. Then redefine
     * the node to be deleted as that leftmost node and replace
     * that leftmost node with its right child. Then rebalance
     * the right subtree if necessary.
     * 
     * Calling parameters:
     * 
     * @param p (IN) the root of the right subtree at this level of recursion
     * @param q (MODIFIED) the node to be deleted
     * 
     * @return the root of the rebalanced subtree
     */
private:
    Node* eraseLeft( Node* p, Node*& q ) {
        
        if ( p->left != nullptr ) {
            p->left = eraseLeft( p->left, q );
            if ( h ) {
                p = balanceEraseLeft( p );
            }
        } else {
            q->key = p->key;                // copy node contents from p to q
            q = p;                          // redefine q as node to be deleted
            p = p->right;                   // replace node with right branch
            h = true;
        }
        return p;  // the root of the rebalanced subtree
    }
    
    /*
     * Replace the node to be deleted with the rightmost node of
     * the left subtree after copying the key from that rightmost
     * node to the key of the node to be deleted. Then redefine
     * the node to be deleted as that rightmost node and replace
     * that rightmost node with its left child. Then rebalance
     * the left subtree if necessary.
     * 
     * Calling parameters:
     * 
     * @param p (IN) the root of the left subtree at this level of recursion
     * @param q (MODIFIED) the node to be deleted
     * 
     * @return the root of the rebalanced left subtree
     */
private:
    Node* eraseRight( Node* p, Node*& q ) {
        
        if ( p->right != nullptr ) {
            p->right = eraseRight( p->right, q );
            if ( h ) {
                p = balanceEraseRight( p );
            }
        } else {
            q->key = p->key;                // copy node contents from p to q
            q = p;                          // redefine q as node to be deleted 
            p = p->left;                    // replace node with left branch
            h = true;
        }
        return p;  // the root of the rebalanced subtree
    }
    
    /*
     * Remove a node from the tree. Then the tree is rebalanced
     * if necessary.
     * 
     * Calling parameters:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * @param x (IN) the key to remove from the tree
     * 
     * @return the root of the rebalanced subtree
     */
public:
    Node* erase( Node* p, K const& x ) {
        
        if ( x < p->key ) {                     // search left branch?
            if ( p->left != nullptr ) {
                p->left = erase( p->left, x );
                if ( h ) {
                    p = balanceEraseLeft( p );
                }
            } else {
                h = false;                      // key is not in the tree
                r = false;
            }
        } else if ( x > p->key ) {              // search right branch?
            if ( p->right != nullptr ) {
                p->right = erase( p->right, x );
                if ( h ) {
                    p = balanceEraseRight( p );
                }
            } else {
                h = false;                      // key is not in the tree
                r = false;
            }
        } else {                                // x == key, so...
            Node* q = p;                        // ...select this node for removal
            if ( p->right == nullptr ) {        // if one branch is nullptr...
                p = p->left;
                h = true;
            } else if ( p->left == nullptr ) {  // ...replace with the other one
                p = p->right;
                h = true;
            } else {                            // otherwise find a node to remove
	        // The node has two children, so replace it
	        // either by the leftmost node of the right subtree
	        // or by the rightmost node of the left subtree.
	        // Select the preferred replacement node from the
	        // taller of the two subtrees.
	        //
	        // Note: INVERT_PREFERRED_TEST is included only
	        // for diagnostic purposes.
#ifdef ENABLE_PREFERRED_TEST
#ifdef INVERT_PREFERRED_TEST
                if ( p->bal < 0)                // left subtree is deeper
#else	      
                if ( p->bal <= 0)               // left or neither subtree is deeper
#endif		  
                {
                    p->left = eraseRight( p->left, q );  // redefine the node to be removed
                    if ( h ) {
                        p = balanceEraseLeft( p );
                    }
                }
                else                            // right subtree is deeper
#endif
                {
                    p->right = eraseLeft( p->right, q );  // redefine the node to be removed
                    if ( h ) {
                        p = balanceEraseRight( p );
                    }
                }
            }
            deleteNode(q);
            r = true;
        }
        return p;  // the root of the rebalanced subtree
    }
    
    /*
     * Check the AVL tree for correctness, i.e.,
     * (1) correct sorted order of keys
     * (2) each node's balance field in the range [-1, 1]
     * 
     * Calling parameters:
     * 
     * @param node (IN) the root of the subtree at this level of recursion
     */
private:
    void checkTree( Node* const node ) {

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

        // Check for correct balance field.
        if ( node->bal > 1 || node->bal < -1 ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " has bal = " << node->bal << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // Descend to the leaves of each subtree.
        if ( node->left != nullptr ) {
            checkTree( node->left );
        }
        if ( node->right != nullptr ) {
            checkTree( node->right );
        }
    }

    /*
     * Check the AVL tree for correctness.
     */
public:
    void checkTree() {

        if (root == nullptr) {
            return;
        }
        checkTree(root);
    }

private:
    void streamNode(Node* const node, std::ostringstream& buffer) {
        if (node != nullptr) {
            buffer << node->key;
        }
    }

private:
    void printNode( Node* const node ) {
        if (node != nullptr) {
            std::cout << node->key;
        }
    }

    /*
     * Print the keys stored in the tree, where the key of
     * the root of the tree is at the left and the keys of
     * the leaf nodes are at the right.
     * 
     * Calling parameters:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
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
        printNode(p);
        std::cout << std::endl;

        if ( p->left != nullptr ) {
            printTree( p->left, d+1 );
        }
    }
    
    /*
     * Print the keys stored in the tree, where the keys of
     * the root of the tree is at the left and the keys of
     * the leaf nodes are at the right.
     */
public:
    void printTree() {
        if ( root != nullptr ) {
            printTree( root, 0 );
        }
    }

    /*
     * Walk the tree in order and store each key in a vector.
     *
     * Calling parameters:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * @param v (MODIFIED) vector of the keys
     * @param i (MODIFIED) index to the next unoccupied vector element
     */       
public:
    void getKeys( Node* const p, std::vector<K>& v, size_t& i ) {

        if ( p->left != nullptr ) {
            getKeys( p->left, v, i );
        }
        v[i++] = p->key;
        if ( p->right != nullptr ) {
            getKeys( p->right, v, i );
        }
    }

    /*
     * Walk the tree in order and store each key in a vector.
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

#endif // ADELSON_VELSKII_LANDIS_WIRTH_AVL_TREE_RECURSE_H
