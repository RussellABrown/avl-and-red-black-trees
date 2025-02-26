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
 * Top-down red-black tree-building program modified from
 * Cullen LaKemper's implementation at the following URL.
 * 
 * https://github.com/SangerC/TopDownRedBlackTree
 *
 * See also the following URLs that discuss top-down insertion and deletion.
 * 
 * https://www.rose-hulman.edu/class/cs/csse230/schedule/day16/Red-Black-Trees-Insertion.pdf
 * https://www.rose-hulman.edu/class/cs/csse230/schedule/day19/Red-Black-Trees-Removal.pdf
 * 
 * The modifications to Cullen LaKemper's red-black tree Java implementation
 * fix a bug in the removeStep2B2 method, remove redundant assignments and
 * tests from the single rotation functions, and transcribe from Java to C++.
 * 
 * To build the test executable, compile via:
 * 
 * g++ -std=c++11 -O3 test_tdrbTree.cpp
 *
 * To disable the freed list that avoids re-use of new and delete, compile via:
 * 
 * g++ -std=c++11 -O3 -D DISABLE_FREED_LIST test_tdrbTree.cpp
 * 
 * To preallocate the freed list as a vector of red-black tree nodes, compile via:
 * 
 * g++ -std=c++11 -O3 -D PREALLOCATE test_tdrbTree.cpp
 * 
 * To enable parent pointers, compile via:
 * 
 * g++ -std=c++11 -O3 -D PARENT test_tdrbTree.cpp
 */

#ifndef CULLEN_LAKEMPER_TDRB_TREE_H
#define CULLEN_LAKEMPER_TDRB_TREE_H

#include <iostream>
#include <exception>
#include <sstream>
#include <vector>

/*
 * The tdrbTree class defines the root of the top-down red-black tree
 * and provides the RED and BLACK bool constants.
 */
template <typename K>
class tdrbTree {

private:
    typedef bool color_t;
    static constexpr color_t RED = true;
    static constexpr color_t BLACK = false;

    /* The Node struct defines a node in the TD RB tree. */
private:
    struct Node
    {
        K key;
        color_t color;
        Node *left, *right;
#ifdef PARENT
        Node* parent;
#endif

        Node( K const& x ) {
            key = x;
            color = RED;
            left = right = nullptr;
        #ifdef PARENT
            parent = nullptr;
        #endif
        }

        Node() {
            color = RED;
            left = right = nullptr;
#ifdef PARENT
            parent = nullptr;
#endif
        }
    };
        
public:
    size_t nodeSize() {
        return sizeof(Node);
    }

private:
    Node* root;     // the root of the tree
    size_t count;   // the number of nodes in the tree

#ifndef DISABLE_FREED_LIST
    Node* freed;    // the freed list
#ifdef PREALLOCATE
    std::vector<Node> nodes;
#endif
#endif

public:
    size_t singleRotationCount, doubleRotationCount;

public:
    tdrbTree() {
        root = nullptr;
        count = singleRotationCount = doubleRotationCount = 0;
        
#ifndef DISABLE_FREED_LIST
        freed = nullptr;
#endif
    }
    
public:
    ~tdrbTree() {
        if ( root != nullptr ) {
            clear();
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
    inline Node* newNode(K const& key) {

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
#ifdef PARENT
            ptr->parent = nullptr;
#endif
            return ptr;
        } else
#endif
        {
            return new Node(key);
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
public:
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
     * Check the tree for correctness, i.e.,
     * (1) no DOUBLE_BLACK nodes
     * (2) no RED child of a RED parent
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
        if ( getColor(node) == RED && getColor(node->right) == RED ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " is RED and right child ";
            streamNode(node->right, buffer);
            buffer << " is also RED" << std::endl;
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
     * Check the RB tree for correctness and count
     * the number of BLACK nodes along each path to the
     * leaves and compare the previous and current counts
     * to ensure that all paths have the same count.
     *
     * @return the number of BLACK nodes along any path
     *         to the bottom of the tree
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
        printNode(p);
        std::cout << std::endl;

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
        if ( p->right != nullptr ) { 
            getKeys( p->right, v, i );
        }
    }

    /* Return the number of nodes in the tree. */
public:
    size_t size() {
        return count;
    }

    /* Return true if there are no nodes in the tree. */
public:
    bool empty() {
        return (count == 0);
    }

private:
    inline color_t getColor( Node* const p ) {
        if (p == nullptr) {
            return BLACK;
        }
        return p->color;
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
        return contains( root, x );
    }
    
    /*
     * Search the tree for the existence of a key
     * and add the key as a new node if not found.
     *
     * Calling parameter:
     *
     * @param n (IN) the key to add to the tree
     * 
     * @return true if the key was added as a new rbNode; otherwise, false
     *
     * WARNING: Declaring this function inline produces a compiler warning
     *          which suggests that inlining it will cause code bloat that
     *          will decrease performance.
     */
public:
    bool insert( K const& n ) {
		if (root == nullptr) {
			root = newNode(n);
		} else {
			Node* ggp = nullptr;
			Node* gp = nullptr;
			Node* p = nullptr;
			Node* m = root;
			
			while (true) {
				testChildrenColors(m, p, gp, ggp);
                int compare = compareTo(n, m->key);
                // Don't insert the key twice.
				if ( compare == 0 ) {
					root->color = BLACK;
					return false;
				}
				
				if (m->left == nullptr && m->right == nullptr) {
					if ( compare < 0 ) {
						addToLeft(m, p, gp, n);
						break;
					}
					else if ( compare > 0 ) {
						addToRight(m, p, gp, n);
						break;
					}
				}
			
				if (m->left == nullptr) {
					if ( compare < 0 ) { 
						addToLeft(m, p, gp, n);
						break;
					}
				} else if (m->right == nullptr) { 
					if ( compare > 0) {
						addToRight(m, p, gp, n);
						break;
					}
				}
				ggp = gp;
				gp = p;
				p = m;
				if ( compare > 0 ) { 
					m = m->right;
				} else {
					m = m->left;
				}
			}
		}
		
		root->color = BLACK;
		++count;
		return true;
	}

private:
    inline int compareTo(K const& k1, K const& k2) {
        if (k1 < k2) {
            return -1;
        } else if (k1 > k2) {
            return 1;
        } else {
            return 0;
        }
    }
	
private:
    inline void testChildrenColors(Node* m, Node* p, Node* gp, Node* ggp) { 
		if (m->left == nullptr || m->right == nullptr) {
            return;
        }
		if (m->right->color == RED && m->left->color == RED) {
            // flip colors
			m->color = RED;
			m->right->color = BLACK;
			m->left->color = BLACK;
			if (p != nullptr) {
				if (p->color == RED && gp != nullptr) {
					Node* x;
                    if (p->key == gp->left->key) {
                        if (m->key == p->left->key) {
                            x = singleRightRotation(gp);
                        } else {
                            x = doubleRightRotation(gp);
                        }
					} else {
                        if (m->key == p->right->key) {
                            x = singleLeftRotation(gp);
                        } else {
                            x = doubleLeftRotation(gp);
                        }
					}
					if (ggp == nullptr) {
                        root = x;
                    } else {
                        setBySide(gp, ggp, x);
                    }
				}
			}
		}
	}
	
private:
    inline void addToLeft(Node* m, Node* p, Node* gp, K const& n) { 
		m->left = newNode(n);
#ifdef PARENT
        m->left->parent = m;
#endif
		if (m->color == RED && p != nullptr) { 
			Node* x;
			if (p->left == nullptr) {
                x = doubleLeftRotation(p);
            } else if (m->key == p->left->key) {
                x = singleRightRotation(p);
            } else {
                x = doubleRightRotation(p);
            }
			if (gp == nullptr) {
                root = x;
            } else {
                setBySide(p, gp, x);
            }
		}
	}

private:
    inline void addToRight(Node* m, Node* p, Node* gp, K const& n) { 
		m->right = newNode(n);
#ifdef PARENT
        m->right->parent = m;
#endif
		if (m->color == RED && p != nullptr) { 
			Node* x;
            if (p->right == nullptr) {
                x = doubleRightRotation(p);
            } else if (m->key == p->right->key) {
                x = singleLeftRotation(p);
            } else {
                x = doubleLeftRotation(p);
            }
			if (gp == nullptr) {
                root = x;
            } else {
                setBySide(p, gp, x);
            }
		}
	}

private:
    inline void setBySide(Node* p, Node* gp, Node* x) { 
		if (gp->right == nullptr) {
            gp->left = x;
        } else if (gp->left == nullptr) {
            gp->right = x;
        } else {
            if (gp->left->key == p->key) {
                gp->left = x;
            } else {
                gp->right = x;
            }
		}
	}

private:
#ifdef PARENT
    inline Node* singleLeftRotation(Node* x) {
        Node* n = x->right;
        x->right = n->left;
        if (x->right != nullptr) {
            x->right->parent = x;
        }
        n->parent = x->parent;
        n->left = x;
        x->parent = n;
        n->color = BLACK;
        if (x->right != nullptr) {
            n->right->color = RED;
        }
        n->left->color = RED;
        ++singleRotationCount;
        return n;
    }
#else
    inline Node* singleLeftRotation(Node* x) {
        Node* n = x->right;
        x->right = n->left;
        n->left = x;
        n->color = BLACK;
        if (x->right != nullptr) {
            n->right->color = RED;
        }
        n->left->color = RED;
        ++singleRotationCount;
        return n;
    }
#endif

private:
#ifdef PARENT
    inline Node* singleRightRotation(Node* x) {
        Node* n = x->left;
        x->left = n->right;
        if (x->left != nullptr) {
            x->left->parent = x;
        }
        n->parent = x->parent;
        n->right = x;
        x->parent = n;
        n->color = BLACK;
        n->right->color = RED;
        if (x->left != nullptr) {
            n->left->color = RED;
        }
        ++singleRotationCount;
        return n;
    }
#else
    inline Node* singleRightRotation(Node* x) {
        Node* n = x->left;
        x->left = n->right;
        n->right = x;
        n->color = BLACK;
        n->right->color = RED;
        if (x->left != nullptr) {
            n->left->color = RED;
        }
        ++singleRotationCount;
        return n;
    }
#endif
    
private:
    inline Node* doubleRightRotation(Node* x) {
        if (x->left != nullptr) {
            x->left = singleLeftRotation(x->left);
            if (x->left->right != nullptr) {
                x->left->right->color = BLACK;
            }
        } else {
            // Avoid incrementing doubleRotationCount.
            return singleRightRotation(x);
        }

        // singleRotationCount is incremented twice
        // for each increment of doubleRotationCount,
        // so to obtain the singleRotationCount distinct
        // from doubleRotationCount, subtract twice the
        // doubleRotationCount from singleRotationCount.
        ++doubleRotationCount;
        return singleRightRotation(x);
    }

private:
    inline Node* doubleLeftRotation(Node* x) {
        if (x->right != nullptr) {
            x->right = singleRightRotation(x->right);
            if (x->right->left != nullptr) {
                x->right->left->color = BLACK;
            }
        } else {
            // Avoid incrementing doubleRotationCount.
            return singleLeftRotation(x);
        }

        // singleRotationCount is incremented twice
        // for each increment of doubleRotationCount,
        // so to obtain the singleRotationCount distinct
        // from doubleRotationCount, subtract twice the
        // doubleRotationCount from singleRotationCount.
        ++doubleRotationCount;
        return singleLeftRotation(x);
    }

    // Step 2 - the main case.
private:
    bool removeStep2(Node* x, K const& n, Node* p, Node* gp) {
        // If x has at least one RED child, continue to Step 2B.
        if (x->left != nullptr && x->left->color == RED) {
            return removeStep2B(x, n, p, gp);
        }
        if (x->right != nullptr && x->right->color == RED) {
            return removeStep2B(x, n, p, gp);
        }
        // x has two BLACK children, so continue to Step 2A.
        return removeStep2A(x, n, p, gp);
    }

    // Step 2A - x has two BLACK children.
private:
    bool removeStep2A(Node* x, K const& n, Node* p, Node* gp) {
        // NOTE that if the sibling has two RED children, it is possible
        // to continue either Step 2A2 or Step 2A3. The order of application
        // of the tests below selects Step 2A3 in the case of two RED
        // children because Step 2A3 applies a single rotation whereas
        // Step 2A2 applies a double rotation and hence Step 2A3 is
        // less expensive than Step 2A2.
        if (p->right->key == x->key) {
            // x is its parent's right child, so check
            // if its sibling's outer (left) child is RED.
            if (p->left->left != nullptr && p->left->left->color == RED) {
                // The outer child is RED, so continue to Step 2A3.
                return removeStep2A3(x, n, p, gp);
            }
            // Check if its sibling's inner (right) child is RED.
            if (p->left->right != nullptr && p->left->right->color == RED) {
                // The inner is RED, so continue to Step 2A2.
                return removeStep2A2(x, n, p, gp);
            }
            // The sibling has two BLACK children, so continue to Step 2A1.
            return removeStep2A1(x, n, p, gp);
        } else {
            // x is its parent's left child, so check
            // if its siblings outer (right) child is RED.
            if (p->right->right != nullptr && p->right->right->color == RED) {
                // The outer child is RED, so continue to Step 2A3.
                return removeStep2A3(x, n, p, gp);
            }
            // Check if its sibling's inner (left) child is RED.
            if (p->right->left != nullptr && p->right->left->color == RED) {
                // The inner child is RED, so continue to Step 2A2.
                return removeStep2A2(x, n, p, gp);
            }
            // The sibling has two BLACK children, so continue to Step 2A1.
            return removeStep2A1(x, n, p, gp);
        }
    }

    // Step 2A1 - p is RED and x and its sibling each have two BLACK children.
private:
    bool removeStep2A1(Node* x, K const& n, Node* p, Node* gp) {
        // Flip (complement) the colors of p and its children.
        p->color = BLACK;
        if (p->right != nullptr) {
            p->right-> color = RED;
        }
        if (p->left != nullptr) {
            p->left->color = RED;
        }
        int compare = compareTo(n, x->key);
        if (compare==0) {
            // x is to be deleted, so continue to Step 3.
            return removeStep3(x, n, p, gp);
        } else if (compare > 0 && x->right != nullptr) {
            // The node to be deleted is possibly in the right subtree,
            // so move down to that subtree and continue to Step 2.
            return removeStep2(x->right, n, x, p);
        } else if (compare < 0 && x->left != nullptr) {
            // The node to be deleted is possibly in the left subtree,
            // so move down to that subtree and continue to Step 2.
            return removeStep2(x->left, n, x, p);
        } else {
            // The node to be deleted is not in the tree.
            return false;
        }
    }

    // Step 2A2 - x has two BLACK children and its sibling's inner child is RED.
private:
    bool removeStep2A2(Node* x, K const& n, Node* p, Node* gp) { 
        Node* z;
        if (p->right != nullptr && x->key == p->right->key) {
            // x is its parent's right child, so rotate double right about the parent.
            z = doubleRightRotation(p);
        } else if (p->left != nullptr && x->key == p->left->key) {
            // x is its parent's left child, so rotate double left about the parent.
            z = doubleLeftRotation(p);
        } else {
            // x is not the parent's child, so don't rotate (how does this case occur?).
            z = p;
        }
        if (gp == nullptr) {
            // The parent is the root, so replace the root with z.
            root = z;
        } else if (p->key == gp->right->key) {
            // The parent is the grandparent's right child, so replace the child with z.
            gp->right = z;
        } else {
            // The parent is the grandparent's left child, so replace the child with z.
            gp->left = z;
        }
        // Recolor RED x and z, which is the (possibly) rotated proxy for the parent.
        x->color = RED;
        z->color = RED;
        // Recolor the proxy's children BLACK.
        if (z->right != nullptr) {
            z->right->color = BLACK;
        }
        if (z->left != nullptr) {
            z->left->color = BLACK;
        }
        int compare = compareTo(n, x->key);
        if (compare==0) {
            // x is to be deleted, so continue to Step 3.
            return removeStep3(x, n, p, gp);
        } else if (compare > 0 && x->right != nullptr) {
            // The node to be deleted is possibly in the right subtree,
            // so move down to that subtree and continue to Step 2.
            return removeStep2(x->right, n, x, p);
        } else if (compare < 0 && x->left != nullptr) {
            // The node to be deleted is possibly in the left subtree,
            // so move down to that subtree and continue to Step 2.
            return removeStep2(x->left, n, x, p);
        } else {
            // The node to be deleted is not in the tree.
            return false;
        }
    }

    // Step 2A3 - x has two BLACK children and the sibling's outer child is RED.
private:
    bool removeStep2A3(Node* x, K const& n, Node* p, Node* gp) { 
        Node* z;
        if (p->right != nullptr && x->key == p->right->key) {
            // x is its parent's right child, so single right rotate about the parent.
            z = singleRightRotation(p);
        } else if (p->left != nullptr && x->key == p->left->key) {
            // x is its parent's left child, so single left rotate about the parent.
            z = singleLeftRotation(p);
        } else {
            // x is not the parent's child, so don't rotate (how does this case occur?).
            z = p;
        }
        if (gp == nullptr) {
            // The parent is the root, so replace the root with z.
            root = z;
        } else if (gp->right != nullptr && p->key == gp->right->key) {
            // The parent is the grandparent's right child, so replace the child with z.
            gp->right = z;
        } else {
            // The parent is the grandparent's left child, so replace the child with z.
            gp->left = z;
        }
        // Recolor RED x and z, which is the (possibly) rotated proxy for the parent.
        x->color = RED;
        z->color = RED;
        // Recolor the proxy's children BLACK.
        if (z->left != nullptr) {
            z->left->color = BLACK;
        }
        if (z->right != nullptr) {
            z->right->color = BLACK;
        }
        int compare = compareTo(n, x->key);
        if (compare==0) {
            // x is to be deleted, so continue to Step 3.
            return removeStep3(x, n, p, gp);
        } else if (compare > 0 && x->right != nullptr) {
            // The node to be deleted is possibly in the right subtree,
            // so move down to that subtree and continue to Step 2.
            return removeStep2(x->right, n, x, p);
        } else if (compare < 0 && x->left != nullptr) {
            // The node to be deleted is possibly in the right subtree,
            // so move down to that subtree and continue to Step 2.
            return removeStep2(x->left, n, x, p);
        } else {
            // The node to be deleted is not in the tree.
            return false;
        }
    }

    // Step 2B - x has at least one RED child.
private:
    bool removeStep2B(Node* x, K const& n, Node* p, Node* gp) { 
        int compare = compareTo(n, x->key);
        if (compare==0) {
            // x is to be deleted, so continue to Step 3.
            return removeStep3(x, n, p, gp);
        }
        // x is not to be deleted, so move down the tree.
        Node* b;
        if (compare > 0) {
            // The node to be deleted is possibly in the right subtree.
            b = x->right;
        } else {
            // The node to be deleted is possibly in the left subtree.
            b = x->left;
        }
        if (b == nullptr) {
            // The node to be deleted is not in the tree.
            return false;
        }
        if (b->color == RED) {
            // The root of the subtree is RED, so continue to Step 2B1.
            return removeStep2B1(b, n, x, p);
        } else {
            // The root of the subtree is BLACK, so continue to Step 2B2.
            return removeStep2B2(b, n, x, p);
        }
    }

    // Step 2B1 - the root (x) of the subtree is RED.
private:
    bool removeStep2B1(Node* x, K const& n, Node* p, Node* gp) { 
        int compare = compareTo(n, x->key);
        if (compare==0) {
            // x is to be deleted, so continue to Step 3.
            return removeStep3(x, n, p, gp);
        } else if (compare > 0 && x->right != nullptr) {
            // The node to be deleted is possibly in the right subtree,
            // so continue to Step 2.
            return removeStep2(x->right, n, x, p);
        } else if (compare < 0 && x->left != nullptr) {
            // The node to be deleted is possibly in the right subtree,
            // so continue to Step 2.
            return removeStep2(x->left, n, x, p);
        } else {
            // The node to be deleted is not in the tree.
            return false;
        }
    }

    // Step 2B2 - the root (x) of the subtree is BLACK.
private:
    bool removeStep2B2(Node* x, K const& n, Node* p, Node* gp) { 
        Node* b;
        if (p->right != nullptr && p->right->key == x->key) {
            // x is the parent's right child, so single rotate right about the parent.
            b = singleRightRotation(p);
        } else {
            // x is the parent's left child, so single rotate left about the parent.
            b = singleLeftRotation(p);
        }
        if (gp == nullptr) {
            // The parent is the root, so replace the root with b.
            root = b;
        } else if (gp->right != nullptr && p->key == gp->right->key) {
            // The parent is the grandparent's right child, so replace the child with b.
            gp->right = b;
        } else {
            // The parent is the grandparent's left child, so replace the child with b.
            gp->left = b;
        }
        // Recolor BLACK b, which prior to rotation was the parent's sibling
        // and now is the parent's parent (i.e., x's grandparent).
        b->color = BLACK;
        // Recolor b's children BLACK.
        if (b->right != nullptr) {
            b->right->color = BLACK;
        }
        if (b->left != nullptr) {
            b->left->color = BLACK;
        }
        // Recolor the parent RED; as b's child, it was recolored BLACK above.
        p->color = RED;
        // Continue to Step 2 and NOTE that the above single
        // rotation has redefined the grandparent (gp) as b.
        return removeStep2(x, n, p, b);
    }

    // Step 3 - delete x.
private:
    bool removeStep3(Node* x, K const& n, Node* p, Node* gp) { 
        if (x->right != nullptr && x->left != nullptr) { 
            // x has two children, so find the in-order predecessor,
            // which is the rightmost node of the left subtree.
            // NOTE that the in-order successor, which is the leftmost
            // node of the right subtree, would be an alternative.
            Node* v = x->left;
            while (v->right != nullptr) { 
                v = v->right;
            }
            if (x->color == RED) {
                // x is RED, so copy the key from the predecessor into x,
                // move down to the left subtree, and continue to Step 2.
                x->key = v->key;
                removeStep2(x->left, v->key, x, p);
                return true;
            } else {
                // x is BLACK, so make a temporary copy of the predecessor's
                // key, continue to Step 2B using that key, and restore the key.
                // HOWEVER, it does not appear that Step 2B or any subsequent
                // step modifies the key, so copy/restore may be unnecessary.
                K temp = v->key;
                removeStep2B(x, v->key, p, gp);
                x->key = temp;
                return true;
            }
        } else if (x->right == nullptr && x->left == nullptr) {
            // x is a leaf node with no children.
            if (p->right != nullptr && x->key == p->right->key) {
                // x is its parent's right child, so delete the child.
                deleteNode(p->right);
                p->right = nullptr;
            } else {
                // x is its parent's left child, so delete the child.
                deleteNode(p->left);
                p->left = nullptr;
            }
            return true;
        }
        // x has one child because the above cases handled two and no children.
        if (x->right != nullptr) {
            // x has a right child; color it BLACK.
            x->right->color = BLACK;
            if (p->right != nullptr && x->key == p->right->key) {
                // x is the parent's right child, so replace the parent's child
                // with x's right child, thus deleting the parent's child.
                Node* temp = p->right;
                p->right = x->right;
                deleteNode(temp);
            } else {
                // x is the parent's left child, so replace the parent's child
                // with x's right child, thus deleting the parent's child.
                Node* temp = p->left;
                p->left = x->right;
                deleteNode(temp);
            }
            return true;
        } else {
            // x has a left child; color it BLACK.
            x->left->color = BLACK;
            if (p->right != nullptr && x->key == p->right->key) {
                // x is the parent's right child, so replace the parent's child
                // with x's left child, thus deleting the parent's child.
                Node* temp = p->right;
                p->right = x->left;
                deleteNode(temp);
            } else {
                // x is the parent's left child, so replace the parent's child
                // with x's left child, thus deleting the parent's child.
                Node* temp = p->left;
                p->left = x->left;
                deleteNode(temp);
            }
            return true;
        }
    }
		
    /*
     * Remove a node from the tree.
     * 
     * Calling parameter:
     * 
     * @param n (IN) the key to remove from the tree
     * 
     * @return true if the key was removed from the tree; otherwise, false
     */
public:
    inline bool erase(K const& n) {
		if (root == nullptr) {
            return false;
        }
		if (erase(root, n)) {
			--count;
			if (root != nullptr) {
				root->color = BLACK;
			}
			return true;
		}

		if (root != nullptr) {
			root->color = BLACK;
		}
		return false;
	}

    /*
     * Remove a node from the tree.
     * 
     * Calling parameters:
     * 
     * @param x (IN) the root of the tree
     * @param n (IN) the key to remove from the tree
     * 
     * @return true if the key was removed from the tree; otherwise, false
     */
private:
    inline bool erase( Node* x, K const& n ) {
        if (x->left == nullptr && x->right == nullptr) { 
            // The root has no children.
            if (n == x->key) {
                // The root matches the key, so delete it.
                deleteNode(root);
                root = nullptr;
                return true;
            }
            // The node to be deleted is not in the tree.
            return false;
        }
        // The root has at least one child.
        if (x->left == nullptr) {
            // The root has no left child.
            if (n == x->key) {
                // Replace the root with x's right child.
                Node* temp = root;
                root = x->right;
                deleteNode(temp);
                return true;
            }
            // Because the root doesn't have a left child, it must have a right child.
            if (n == x->right->key) {
                // Delete the root's right child. The invariant of the red-black tree
                // guarantees that, because the root has no left child, its right
                // child cannot have a child. Check for this as a test of correctness.
                if (root->right->right != nullptr) {
                    throw std::runtime_error("\nroot has no left child and it's right child has a right child\n");
                }
                if (root->right->left != nullptr) {
                    throw std::runtime_error("\nroot has no left child and it's right child has a left child\n");
                }
                deleteNode(root->right);
                root->right = nullptr;
                return true;
            }
            // The node to be deleted is not in the tree.
            return false;
        }
        // The root has at least one child.
        if (x->right == nullptr) {
            // The root has no right child.
            if (n == x->key) {
                // Replace root with x's left child.
                Node* temp = root;
                root = x->left;
                deleteNode(temp);
                return true;
            }
            // Because the root doesn't have a right child, it must have a left child.
            if (n == x->left->key) {
                // Delete the root's left child. The invariant of the red-black tree
                // guarantees that, because the root has no right child, its left
                // child cannot have a child. Check for this as a test of correctness.
                if (root->left->right != nullptr) {
                    throw std::runtime_error("\nroot has no right child and it's left child has a right child\n");
                }
                if (root->left->left != nullptr) {
                    throw std::runtime_error("\nroot has no right child and it's left child has a left child\n");
                }
                deleteNode(root->left);
                root->left = nullptr;
                return true;
            }
            // The node to be deleted is not in the tree.
            return false;
        }
        // Here is Step 1.
        if (x->left->color == BLACK && x->right->color == BLACK) {
            // Both of the root's children are BLACK, so color the root RED.
            x->color = RED;
            int compare = compareTo(n, x->key);
            if ( n == x->key ) {
                // The root is to be deleted, so continue to Step 3.
                return removeStep3(x, n, nullptr, nullptr);
            } else if ( n > x->key && x->right != nullptr ) {
                // The node to be deleted is possibly in the right subtree,
                // so move down to that subtree and continue to Step 2.
                return removeStep2(x->right, n, x, nullptr);
            } else if ( n < x->key && x->left != nullptr ) {
                // The node to be deleted is possibly in the left subtree,
                // so move down to that subtree and continue to Step 2.
                return removeStep2(x->left, n, x, nullptr);
            } else {
                // The node to be deleted is not in the tree.
                return false;
            }
        } else {
            // The root has at least one RED child, so continue to Step 2B.
            return removeStep2B(x, n, nullptr, nullptr);
        }
    }

    /*
     * Print the keys stored in the tree, where the key of
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

#endif // CULLEN_LAKEMPER_TDRB_TREE_H
