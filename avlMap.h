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
 * AVL map-building functions modified from Pascal procedures
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

#ifndef ADELSON_VELSKII_LANDIS_WIRTH_AVL_MAP_RECURSE_H
#define ADELSON_VELSKII_LANDIS_WIRTH_AVL_MAP_RECURSE_H


#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

/*
 * The avlMap class defines the root of the AVL map and provides the
 * lli, lri, rli, rri, lle, lre, rle, and rre rotation counters
 * and the h, a, and r boolean variables to the avlNode class.
 */
template <typename K, typename V>
class avlMap {

    /* The avlNode class defines a node in the AVL map. */

    class avlNode {

    private:
        K key;          /* the key stored in this avlNode */
        V value;        /* the value stored in this avlNode */
        int bal;        /* the left/right balance that assumes values of -1, 0, or +1 */

    public:
        avlNode *left, *right;
        
        /*
         * Here is the constructor for the avlNode class.
         *
         * Calling parameters:
         * 
         * @param x (IN) the key to store in the avlNode
         * @param h (MODIFIED) specifies that the map height has changed
         */
    public:
        avlNode( K const& x, V const& y, bool& h ) {
            h = true;
            key = x;
            value = y;
            bal = 0;
            left = right = nullptr;
        }
        
        /*
         * This method searches the map for the existence of a key.
         *
         * Calling parameter:
         *
         * @param x (IN) the key to search for
         * 
         * @return true if the key was found; otherwise, false
         */
    public:
        bool contains( K const& x ) {

            if ( find(x) == nullptr ) {
                return false;
            } else {
                return true;
            }
        }

        /*
         * This method searches the map for the existence of a key
         * and returns a pointer to the associated value.
         *
         * The "this" pointer is copied to the "p" pointer that is
         * replaced by either the left or right child pointer as the
         * search iteratively descends through the map.
         * 
         * Calling parameter:
         *
         * @param x (IN) the key to search for
         * 
         * @return a pointer to the value if the key was found; otherwise, nullptr
         */
    public:
        V* find( K const& x ) {
            
            avlNode* p = this;
            
            while ( p != nullptr ) {                    /* iterate; don't use recursion */
                if ( x < p->key ) {
                    p = p->left;                        /* follow the left branch */
                } else if ( x > p->key ) {
                    p = p->right;                       /* follow the right branch */
                } else {
                    return &(p->value);                 /* found the key, so return pointer to value */
                }
            }
            return nullptr;                               /* didn't find the key, so return nullptr */
        }

        
        /*
         * This method searches the map recursively for the existence
         * of a key, and either inserts the (key, value) as a new avlNode
         * or updates the value. Then the map is rebalanced if necessary.
         * 
         * The "this" pointer is copied to the "p" pointer that is
         * possibly modified and is returned to represent the root of
         * the sub-map.
         *
         * Calling parameters:
         *
         * @param m (IN) a pointer to the avlMap instance
         * @param x (IN) the key to add to the map
         * @param y (IN) the value to add to the map
         */
    public:
        avlNode* insert( avlMap* const m, K const& x, V const& y ) {
            
            avlNode* p = this;
            
            if ( x < p->key ) {                         /* search the left branch? */
                if ( p->left != nullptr ) {
                    p->left = left->insert( m, x, y );
                } else {
                    p->left = new avlNode( x, y, m->h );
                    m->a = false;                       /* the present value is overwritten */
                }
                if ( m->h == true ) {                   /* left branch has grown higher */
                    switch ( p->bal ) {
                        case 1:                         /* balance restored */
                            p->bal = 0;
                            m->h = false;
                            break;
                        case 0:                         /* map has become more unbalanced */
                            p->bal = -1;
                            break;
                        case -1:		                /* map must be rebalanced */
                            avlNode* p1 = p->left;
                            if ( p1->bal == -1 ) {		/* single LL rotation */
                                m->lli++;
                                p->left = p1->right;
                                p1->right = p;
                                p->bal = 0;
                                p = p1;
                            } else {			        /* double LR rotation */
                                m->lri++;
                                avlNode* p2 = p1->right;
                                p1->right = p2->left;
                                p2->left = p1;
                                p->left = p2->right;
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
                            m->h = false;
                            break;
                    }
                }
            } else if ( x > p->key ) {                  /* search the right branch? */
                if ( p->right != nullptr ) {
                    p->right = right->insert( m, x, y );
                } else {
                    p->right = new avlNode( x, y, m->h );
                    m->a = false;                       /* the present value is overwritten */
                }
                if ( m->h == true ) {                   /* right branch has grown higher */
                    switch ( p->bal ) {
                        case -1:                        /* balance restored */
                            p->bal = 0;
                            m->h = false;
                            break;
                        case 0:                         /* map has become more unbalanced */
                            p->bal = 1;
                            break;
                        case 1:                         /* map must be rebalanced */
                            avlNode* p1 = p->right;
                            if ( p1->bal == 1 ) {       /* single RR rotation */
                                m->rri++;
                                p->right = p1->left;
                                p1->left = p;
                                p->bal = 0;
                                p = p1;
                            } else {                    /* double RL rotation */
                                m->rli++;
                                avlNode* p2 = p1->left;
                                p1->left = p2->right;
                                p2->right = p1;
                                p->right = p2->left;
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
                            m->h = false;
                            break;
                    }
                }
            } else {  /* the key is already in map, so update its value */
                p->value = y;
                m->h = false;
                m->a = true;
            }
            return p;  /* the root of the rebalanced sub-map */
        }
        
        /*
         * This method rebalances following deletion of a
         * left avlNode.
         * 
         * The "this" pointer is copied to the "p" pointer that is
         * possibly modified and is returned to represent the root of
         * the sub-map.
         * 
         * Calling parameters:
         * 
         * @param m (IN) a pointer to the avlMap instance
         * 
         * @return the root of the rebalanced sub-map
         */
    private:
        avlNode* balanceLeft( avlMap* const m ) {
            
            avlNode* p = this;
            
            switch ( p->bal ) {
                case -1:                    /* balance restored */
                    p->bal = 0;
                    break;
                case 0:                     /* map has become more unbalanced */
                    p->bal = 1;
                    m->h = false;
                    break;
                case 1:                     /* map must be rebalanced */
                    avlNode* p1 = p->right;
                    if ( p1->bal >= 0 ) {   /* single RR rotation */
                        m->rre++;
                        p->right = p1->left;
                        p1->left = p;
                        if ( p1->bal == 0 ) {
                            p->bal = 1;
                            p1->bal = -1;
                            m->h = false;
                        } else {
                            p->bal = 0;
                            p1->bal = 0;
                        }
                        p = p1;
                    } else {				  /* double RL rotation */
                        m->rle++;
                        avlNode* p2 = p1->left;
                        p1->left = p2->right;
                        p2->right = p1;
                        p->right = p2->left;
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
            return p; /* the root of the rebalanced sub-map */
        }
        
        /*
         * This method rebalances following deletion of a
         * right avlNode.
         * 
         * The "this" pointer is copied to the "p" pointer that is
         * possibly modified and is returned to represent the root of
         * the sub-map.
         * 
         * Calling parameters:
         * 
         * @param m (IN) a pointer to the avlMap instance
         * 
         * @return the root of the rebalanced sub-map
         */
    private:
        avlNode* balanceRight( avlMap* const m ) {
            
            avlNode* p = this;
            
            switch ( p->bal ) {
                case 1:                     /* balance restored */
                    p->bal = 0;
                    break;
                case 0:                     /* map has become more unbalanced */
                    p->bal = -1;
                    m->h = false;
                    break;
                case -1:                    /* map must be rebalanced */
                    avlNode* p1 = p->left;
                    if ( p1->bal <= 0 ) {   /* single LL rotation */
                        m->lle++;
                        p->left = p1->right;
                        p1->right = p;
                        if ( p1->bal == 0 ) {
                            p->bal = -1;
                            p1->bal = 1;
                            m->h = false;
                        } else {
                            p->bal = 0;
                            p1->bal = 0;
                        }
                        p = p1;
                    } else {				  /* double LR rotation */
                        m->lre++;
                        avlNode* p2 = p1->right;
                        p1->right = p2->left;
                        p2->left = p1;
                        p->left = p2->right;
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
            return p;  /* the root of the rebalanced sub-map */
        }
        
        /*
         * This method replaces the avlNode to be deleted with the
         * leftmost avlNode of the right sub-map. Then the map is
         * rebalanced if necessary.
         * 
         * The "this" pointer is copied to the "p" pointer that is
         * possibly modified and is returned to represent the root of
         * the sub-map.
         * 
         * Calling parameters:
         * 
         * @param m (IN) a pointer to the avlMap instance
         * @param q (MODIFIED) the avlNode to be deleted
         * 
         * @return the root of the rebalanced sub-map
         */
     private:
        avlNode* eraseLeft( avlMap* const m, avlNode*& q ) {
            
            avlNode* p = this;
            
            if ( p->left != nullptr ) {
                p->left = left->eraseLeft( m, q );
                if ( m->h == true ) p = balanceLeft( m );
            } else {
                q->key = p->key;                /* copy avlNode contents from p to q */
                q->value = p->value;
                q = p;                          /* redefine q as avlNode to be deleted */
                p = p->right;                   /* replace avlNode with right branch */
                m->h = true;
            }
            return p;  /* the root of the rebalanced sub-map */
        }
        
        /*
         * This method replaces the avlNode to be deleted with the
         * rightmost avlNode of the left sub-map. Then the map is
         * rebalanced if necessary.
         * 
         * The "this" pointer is copied to the "p" pointer that is
         * possibly modified and is returned to represent the root of
         * the sub-map.
         * 
         * Calling parameters:
         * 
         * @param m (IN) a pointer to the avlMap instance
         * @param q (MODIFIED) the avlNode to be deleted
         * 
         * @return the root of the rebalanced sub-map
         */
    private:
        avlNode* eraseRight( avlMap* const m, avlNode*& q ) {
            
            avlNode* p = this;
            
            if ( p->right != nullptr ) {
                p->right = p->right->eraseRight( m, q );
                if ( m->h == true ) p = balanceRight( m );
            } else {
                q->key = p->key;                /* copy avlNode contents from p to q */
                q->value = p->value;
                q = p;                          /* redefine q as avlNode to be deleted  */
                p = p->left;                    /* replace avlNode with left branch */
                m->h = true;
            }
            return p;  /* the root of the rebalanced sub-map */
        }
        
        /*
         * This method removes an avlNode from the map. Then
         * the map is rebalanced if necessary.
         * 
         * The "this" pointer is copied to the "p" pointer that is
         * possibly modified and is returned to represent the root of
         * the sub-map.
         * 
         * Calling parameters:
         * 
         * @param m (IN) a pointer to the avlMap instance
         * @param x (IN) the key to remove from the map
         * 
         * @return the root of the rebalanced sub-map
        */
     public:
        avlNode* erase( avlMap* const m, K const& x ) {
            
            avlNode* p = this;
            
            if ( x < p->key ) {                     /* search left branch? */
                if ( p->left != nullptr ) {
                    p->left = p->left->erase( m, x );
                    if ( m->h ) {
                        p = balanceLeft( m );
                    }
                } else {
                    m->h = false;                   /* key is not in the map*/
                    m->r = false;
                }
            } else if ( x > p->key ) {              /* search right branch? */
                if ( p->right != nullptr ) {
                    p->right = p->right->erase( m, x );
                    if ( m->h ) {
                        p = balanceRight( m );
                    }
                } else {
                    m->h = false;                   /* key is not in the map */
                    m->r = false;
                }
            } else {                                /* the search key equals the key for this node */
                avlNode* q = p;                     /* so select this avlNode for removal */
                if ( p->right == nullptr ) {        /* if one branch is nullptr... */
                    p = p->left;
                    m->h = true;
                } else if ( p->left == nullptr ) {  /* ...replace with the other one */
                    p = p->right;
                    m->h = true;
                } else {
                    switch ( p->bal ) {             /* otherwise find an avlNode to remove */
                        case 0: case -1:            /* left or neither submap is deeper */
                            p->left = p->left->eraseRight( m, q );
                            if ( m->h == true ) {
                                p = balanceLeft( m );
                            }
                            break;
                        case 1:                     /* right submap is deeper */
                            p->right = p->right->eraseLeft( m, q );
                            if ( m->h == true ) {
                                p = balanceRight( m );
                            }
                            break;
                        default:
                            {
                                std::ostringstream buffer;
                                buffer << std::endl << (p->bal) << " is out of range" << std::endl;
                                throw std::runtime_error(buffer.str());
                            }
                    }
                }
                delete q;
                m->r = true;
            }
            return p;  /* the root of the rebalanced sub-map */
        }
        
        /*
         * This method prints the keys stored in the map, where the
         * key of the root of the map is at the left and the keys of
         * the leaf nodes are at the right.
         * 
         * Calling parameter:
         * 
         * @param d (MODIFIED) the depth in the map
         */
    public:
        void printMap( int d ) {
            if ( right != nullptr ) {
                right->printMap( d+1 );
            }
            for ( int i = 0; i < d; ++i ) {
                std::cout << "    ";
            }
            std::cout << key << "\n";
            if ( left != nullptr ) {
                left->printMap( d+1 );
            }
        }
        
        /*
         * This method deletes every avlNode in the map.  If the
         * map has been completely deleted via prior calls to the
         * erase() method, the ~avlMap() destructor will not call
         * this method.
         */
    public:
        void clear() {
            
            if ( left != nullptr ) {
                left->clear();
                left = nullptr;
            }
            if ( right != nullptr ){
                right->clear();
                right = nullptr;
            }
            delete this;
        }

        /*
         * This method walks the map in order and stores each key in a vector.
         *
         * Calling parameters:
         * 
         * @param v (MODIFIED) vector of the keys
         * @param i (MODIFIED) index to the next unoccupied vector element
         */       
    public:
        void getKeys( std::vector<K>& v, size_t& i ) {

            if ( left != nullptr ) {
                left->getKeys( v, i );
            }
            v[i++] = this->key;
            if ( right != nullptr ){
                right->getKeys( v, i );
            }
        }
    };

private:
    avlNode* root;  /* the root of the map */
    size_t count;   /* the number of nodes in the map */
    bool h, a, r;   /* record modification of the map */

public:
    size_t lle, lre, rle, rre, lli, lri, rli, rri;  /* the rotation counters */
   
public:
    avlMap() {
        root = nullptr;
        lle = lre = rle = rre = lli = lri = rli = rri = count = 0;
        h = a = r = false;
    }
    
public:
    ~avlMap() {
        if ( root != nullptr ) {
            root->clear();
        }
    }

    /* This method returns the number of avlNodes in the map. */
public:
    size_t size() {
        return count;
    }

    /* This method returns true if there are no avlNodes in the map. */
public:
    bool empty() {
        return ( count == 0 );
    }

    /* This method searches the map for the existence of a key.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to search for
     * 
     * @return true if the key was found; otherwise, false
     */
public:
    bool contains( K const& x ) {
        if (root != nullptr) {
            return root->contains( x );
        } else {
            return false;
        }
    }
    
    /* This method searches the map for the existence of a key
     * and returns the associated value.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to search for
     * 
     * @return a pointer to the value if the key was found; otherwise, nullptr */
public:
    V* find( K const& x ) {
        if (root != nullptr) {
            return root->find( x );
        } else {
            return nullptr;
        }
    }
    
    /*
     * This method searches the map recursively for the existence
     * of a key, and either inserts the (key, value) as a new avlNode
     * or updates the value. Then the map is rebalanced if necessary.
     *
     * Calling parameter:
     *
     * @param x (IN) the key to add to the map
     * 
     * @return true if update, false if insertion
     */
public:
    bool insert( K const& x, V const& y ) {
        h = false, a = false;
        if ( root != nullptr ) {
            root = root->insert( this, x, y );
            if ( a == false ) {
                ++count;
            }
        } else {
            root = new avlNode( x, y, h );
            ++count;
        }
        return a;
    }

    /*
     * This method removes an avlNode from the map.
     * Then the map is rebalanced if necessary.
     * 
     * Calling parameter:
     * 
     * @param x (IN) the key to remove from the map
     * 
     * @return true if the key existed, false if not
     */
public:
    bool erase( K const& x ) {
        h = false, r = false;
        if ( root != nullptr ) {
            root = root->erase( this, x );
            if ( r == true ) {
                --count;
            }
        }
        return r;
    }

    /*
     * This method prints the keys stored in the map, where the
     * key of the root of the map is at the left and the keys of
     * the leaf nodes are at the right.
     */
public:
    void printMap() {
        if ( root != nullptr ) {
            root->printMap( 0 );
        }
    }

    /* This method deletes every avlNode in the AVL map. */
public:
    void clear() {
        if ( root != nullptr ) {
            root->clear();
        }
        root = nullptr;
        count = 0;
    }

    /*
     * This method walks the map in order and stores each key in a vector.
     *
     * Calling parameter:
     * 
     * @param v (MODIFIED) vector of the keys
     */
public:
    void getKeys( std::vector<K>& v ) {
        if ( root != nullptr ) {
            size_t i = 0;
            root->getKeys( v, i );
        }
    }
};

#endif // ADELSON_VELSKII_LANDIS_WIRTH_AVL_MAP_RECURSE_H
