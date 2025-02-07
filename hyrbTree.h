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
 * Hybrid top-down bottom-up red-black tree-building program
 * that combines top-down insertion with bottom-up deletion.
 * 
 * See Cullen LaKemper's top-down Java implementation at the following URL.
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
 * See also Rao H Ananda's C++ bottom-up implementation at the following URl.
 * 
 * https://github.com/anandarao/Red-Black-Tree
 * 
 * The modifications to Rao Ananda's red-black tree implementation
 * fix bugs in and eliminate memory leaks from the fixDeleteRBT
 * function, which is renamed to the fixErasure function.
 *
 * Also, the modifications replace recursion with iteration
 * in the insert and erase functions.
 * 
 * To build the test executable, compile via:
 * 
 * g++ -std=c++11 -O3 test_hyrbTree.cpp
 *
 * To disable the freed list that avoids re-use of new and delete, compile via:
 * 
 * g++ -std=c++11 -O3 -D DISABLE_FREED_LIST test_hyrbTree.cpp
 * 
 * To use a non-static sentinel node nullnode instead of nullptr, compile via:
 * 
 * g++ -std=c++11 -O3 -D NULL_NODE test_hyrbTree.cpp
 * 
 * To use a static sentinel node nullnode instead of nullptr,
 * NOTE that C++17 is required and compile via:
 * 
 * g++ -std=c++17 -O3 -D STATIC_NULL_NODE test_hyrbTree.cpp
 */

#ifndef LAKEMPER_ANANDA_HYBRID_RB_TREE_H
#define LAKEMPER_ANANDA_HYBRID_RB_TREE_H

#include <cstdint>
#include <iostream>
#include <exception>
#include <sstream>
#include <vector>

/*
 * The hyrbTree class defines the root of the hybrid red-black tree
 * and provides the RED, BLACK, and DOUBLE_BLACK uint8_t constants.
 */
template <typename K>
class hyrbTree
{

    /*
     * NULL_NODE or STATIC_NULL_NODE selects the sentinel node
     * nullnode, which does not require that a node pointer be
     * checked for nullptr before setting a field of that node
     * and hence may improve performance.
     */
#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
#define nulle nullnode
#else
#define nulle nullptr
#endif

    /* Don't rely on the compiler to use int for an enum. */
private:
    typedef uint8_t color_t;
    static constexpr color_t RED = 0;
    static constexpr color_t BLACK = 1;
    static constexpr color_t DOUBLE_BLACK = 2;

    /* The Node struct defines a node in the TD RB tree. */
#ifdef STATIC_NULL_NODE
public:
#else
private:
#endif
    struct Node
    {
        K key;
        color_t color;
        Node *left, *right, *parent;

public:
        Node() {
            color = RED;
            left = right = parent = nullptr;
        }

public:
        Node(K const& x) {
            key = x;
            color = RED;
            left = right = parent = nullptr;
        }
    };

    /*
     * Here is an initializer for a node's pointers
     * because the Node() constructor does not recognize
     * nullnode when nullnode is defined as a Node pointer.
     * 
     * @return a pointer to a node
     */
private:
    Node* createNode() {
        Node* temp = new Node();
        temp->left = temp->right = temp->parent = nulle;
        return temp;
    }

    /*
     * Here is an initializer for a node's pointers
     * because the Node() constructor does not recognize
     * nullnode when nullnode is defined as a Node pointer.
     * 
     * This alternate contructor accepts a key argument.
     * 
     * Calling parameter:
     * 
     * @param key (IN) - the key to store in the node
     * 
     * @return a pointer to a node
     */
private:
    Node* createNode(K const& key) {
        Node* temp = new Node(key);
        temp->left = temp->right = temp->parent = nulle;
        return temp;
    }

#ifdef STATIC_NULL_NODE
public:
    inline static Node* nullnode; // inline requires C++ 17
#else
#ifdef NULL_NODE
private:
    Node* nullnode;
#endif
#endif

        
private:
    Node* root;
    size_t count; // the number of nodes in the tree

#ifndef DISABLE_FREED_LIST
    Node* freed;
#endif

public:
    size_t singleRotationCount, doubleRotationCount, rotateL, rotateR;

    /* Here is the hyrbTree constructor, which must
     * initialize nullnode first so that root and freed
     * will be initialized to the value of nullnode.
     * 
     * It is not possible to initialize nullnode via an
     * initializer list, for example:
     *
     *      hyrbTree() : nullnode = new Node()
     *
     * because the Node struct and hence its constructor
     * are not available until an instance of hyrbTree
     * has been created by its constructor. And for the
     * same reason, it is not possible to assign hyrbTree
     * member fields from the nullnode member field within
     * the constructor. 
     *
     * See the createNode member function above that initializes
     * member fields of hyrbTree from nullnode. createNode
     * is possible because, after a hyrbTree instance has been
     * created, both createNode and nullnode are accessible.
     * 
     * NOTE that because this hyrbTree constructor creates
     * the nullnode Node instance, the ~hyrbTree destructor
     * must delete nullnode to avoid a memory leak.
     */
public:
    hyrbTree() {

#if defined(NULL_NODE) && !defined(STATIC_NULL_NODE)
        nulle = new Node();
#endif
        root = nulle;
        count = singleRotationCount = doubleRotationCount = rotateL = rotateR = 0;

#ifndef DISABLE_FREED_LIST
        freed = nulle;
#endif
    }
    
public:
    ~hyrbTree() {
        if ( root != nulle ) {
            clear();
        }

#if defined(NULL_NODE) && !defined(STATIC_NULL_NODE)
        delete nullnode;
#endif
    }

public:
    size_t nodeSize() {
        return sizeof(Node);
    }

    /*
     * Delete every node in the HY RB subtree.  If the tree has been
     * completely deleted via prior calls to the erase function,
     * this function will do nothing.
     * 
     * Calling parameter:
     * 
     * @param node (IN) pointer to a node
     */
private:
    void clear(Node* const node) {
        if (node == nulle) {
            return;
        }
        clear(node->left);
        clear(node->right);
        delete node;
    }

    /*
     * Delete every node in the HY RB tree.  If the tree has been
     * completely deleted via prior calls to the erase function,
     * this function will do nothing.
     */
public:
    void clear() {
	    clear(root);
        root = nulle;
        count = 0;
#ifndef DISABLE_FREED_LIST
	    clearFreed();
#endif
    }

    /* Delete every node from the freed list. */
private:
    void clearFreed() {
#ifndef DISABLE_FREED_LIST
        while ( freed != nulle ) {
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
        while(ptr != nulle) {
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
       for (size_t i = 0; i < n; ++i) {
            Node* p = createNode();
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
private:
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
        if (freed != nulle )
        {
            Node* ptr = freed;
            freed = freed->left;
            ptr->key = key;
            ptr->color = RED;
            ptr->left = ptr->right = ptr->parent = nulle;
            return ptr;
        } else
#endif
        {
            return createNode(key);
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
        while ( p != nulle ) {                    /* iterate; don't use recursion */
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
		if (root == nulle) {
			root = newNode(n);
		} else {
			Node* ggp = nulle;
			Node* gp = nulle;
			Node* p = nulle;
			Node* m = root;
			
			while (true) {
				testChildrenColors(m, p, gp, ggp);
                int compare = compareTo(n, m->key);
                // Don't insert the key twice.
				if ( compare == 0 ) {
					root->color = BLACK;
					return false;
				}
				
				if (m->left == nulle && m->right == nulle) {
					if ( compare < 0 ) {
						addToLeft(m, p, gp, n);
						break;
					}
					else if ( compare > 0 ) {
						addToRight(m, p, gp, n);
						break;
					}
				}
			
				if (m->left == nulle) {
					if ( compare < 0 ) { 
						addToLeft(m, p, gp, n);
						break;
					}
				} else if (m->right == nulle) { 
					if ( compare > 0) {
						addToRight(m, p, gp, n);
						break;
					}
				}
				ggp = gp;
				gp =p;
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
		if (m->left == nulle || m->right == nulle) {
            return;
        }
		if (m->right->color == RED && m->left->color == RED) {
            // flip colors
			m->color = RED;
			m->right->color = BLACK;
			m->left->color = BLACK;
			if (p != nulle) {
				if (p->color == RED && gp != nulle) {
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
					if (ggp == nulle) {
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
        m->left->parent = m;
		if (m->color == RED && p != nulle) { 
			Node* x;
			if (p->left == nulle) {
                x = doubleLeftRotation(p);
            } else if (m->key == p->left->key) {
                x = singleRightRotation(p);
            } else {
                x = doubleRightRotation(p);
            }
			if (gp == nulle) {
                root = x;
            } else {
                setBySide(p, gp, x);
            }
		}
	}

private:
    inline void addToRight(Node* m, Node* p, Node* gp, K const& n) { 
		m->right = newNode(n);
        m->right->parent = m;
		if (m->color == RED && p != nulle) { 
			Node* x;
            if (p->right == nulle) {
                x = doubleRightRotation(p);
            } else if (m->key == p->right->key) {
                x = singleLeftRotation(p);
            } else {
                x = doubleLeftRotation(p);
            }
			if (gp == nulle) {
                root = x;
            } else {
                setBySide(p, gp, x);
            }
		}
	}

    /*
     * Assign pointers for the root of the subtree created
     * by addToRight, addToLeft, and testChildColors.
     * 
     * Calling parameters:
     * 
     * @param p (IN) the parent of the root of the subtree
     * @param gp (IN) the grandparent of the root of the subtree
     * @param x (IN) the root of the subtree
     */
private:
    inline void setBySide(Node* p, Node* gp, Node* x) { 
		if (gp->right == nulle) {
            gp->left = x;
            x->parent = gp;
        } else if (gp->left == nulle) {
            gp->right = x;
            x->parent = gp;
        } else {
            if (gp->left->key == p->key) {
                gp->left = x;
                x->parent = gp;
            } else {
                gp->right = x;
                x->parent = gp;
            }
		}
	}

    /*
     * Rotate left at a node, analogous to the RR rotation
     * of the AVL tree. Used for insertion. Assign neither
     * a parent pointer to the new root of the subtree nor
     * a pointer from the parent to that new root because
     * because those assignments cause errors in setBySide.
     * Instead, assign those pointers in setBySide.
     * 
     * Calling parameter:
     * 
     * @param x (IN) the node at which to rotate left
     * 
     * @return the new root of the subtree created by rotation
     */
private:
    inline Node* singleLeftRotation(Node* x) {

        Node* n = x->right;
        x->right = n->left;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        x->right->parent = x;
#else
        if (x->right != nulle) {
            x->right->parent = x;
        }
#endif
        // Check whether x is the root but otherwise there is
        // no need to assign a parent pointer to n because
        // setBySide will do it.
        if (x == root) {  // equivalent to x->parent == nulle
            root = n;
            n->parent = nulle;
        }

        // Assign pointers for x.
        n->left = x;
        x->parent = n;

        n->color = BLACK;
        n->left->color = RED;

        if (x->right != nulle) {
            n->right->color = RED;
        }

        ++singleRotationCount;
        return n;
    }

    /*
     * Rotate left at a node, analogous to the RR rotation
     * of the AVL tree. Used for insertion. Assign a parent
     * pointer to the new root of the subtree and a pointer
     * the parent to that new root. 
     * 
     * Calling parameter:
     * 
     * @param x (IN) the node at which to rotate left
    * 
     * @return the new root of the subtree created by rotation
     */
private:
    inline Node* singleLeftRotation1(Node* x) {

        Node* n = x->right;
        x->right = n->left;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        x->right->parent = x;
#else
        if (x->right != nulle) {
            x->right->parent = x;
        }
#endif

        n->parent = x->parent;

        // No need to check whether x is the root because
        // this singleLeftRotation function is called from
        // only doubleRightRotation; hence, x is a left child.
        if (x == x->parent->left) {
            x->parent->left = n;
        } else {
            x->parent->right = n;
        }

        // Assign pointers for x.
        n->left = x;
        x->parent = n;

        n->color = BLACK;
        n->left->color = RED;

        if (x->right != nulle) {
            n->right->color = RED;
        }

        ++singleRotationCount;
        return n;
    }

    /*
     * Rotate left at a node, analogous to the RR rotation
     * of the AVL tree. Used for insertion. Assign neither
     * a parent pointer to the new root of the subtree nor
     * a pointer from the parent to that new root because
     * because those assignments cause errors in setBySide.
     * Instead, assign those pointers in setBySide.
     * 
     * Also, do not perform some tests and assignments
     * performed by the singleRightRotation function
     * because this singleRightRotation2 function is
     * called only after singleRightRotation1 that
     * renders those tests and assignments redundant.
     * 
     * Calling parameter:
     * 
     * @param x (IN) the node at which to rotate left
     * 
     * @return the new root of the subtree created by rotation
     */
private:
    inline Node* singleLeftRotation2(Node* x) {

        Node* n = x->right;
        x->right = n->left;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        x->right->parent = x;
#else
        if (x->right != nulle) {
            x->right->parent = x;
        }
#endif

        // Check whether x is the root but otherwise there is
        // no need to assign a parent pointer to n because
        // setBySide will do it.
        if (x == root) {  // equivalent to x->parent == nulle
            root = n;
            n->parent = nulle;
        }

        // Assign pointers for x.
        n->left = x;
        x->parent = n;

        // The colors of n and n->right have already been
        // assigned by the singleLeftRotation1 function.
        n->left->color = RED;

        ++singleRotationCount;
        return n;
    }

    /*
     * Rotate right at a node, analogous to the LL rotation
     * of the AVL tree. Used for insertion. Assign neither
     * a parent pointer to the new root of the subtree nor
     * a pointer from the parent to that new root because
     * because those assignments cause errors in setBySide.
     * Instead, assign those pointers in setBySide. 
     * 
     * Calling parameter:
     * 
     * @param x (IN) the node at which to rotate right
     * 
     * @return the new root of the subtree created by rotation
     */
private:
    inline Node* singleRightRotation(Node* x) {

        Node* n = x->left;
        x->left = n->right;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        x->left->parent = x;
#else
        if (x->left != nulle) {
            x->left->parent = x;
        }
#endif
        // Check whether x is the root but otherwise there is
        // no need to assign a parent pointer to n because
        // setBySide will do it.
        if (x == root) {  // equivalent to x->parent == nulle
            root = n;
            n->parent = nulle;
        }

        // Assign pointers for x.
        n->right = x;
        x->parent = n;

        n->color = BLACK;
        n->right->color = RED;

        if (x->left != nulle) {
            n->left->color = RED;
        }

        ++singleRotationCount;
        return n;
    }
    
    /*
     * Rotate right at a node, analogous to the LL rotation
     * of the AVL tree. Used for insertion. Assign a parent
     * pointer to the new root of the subtree and a pointer
     * the parent to that new root. 
     * 
     * Calling parameter:
     * 
     * @param x (IN) the node at which to rotate right
     * 
     * @return the new root of the subtree created by rotation
     */
private:
    inline Node* singleRightRotation1(Node* x) {

        Node* n = x->left;
        x->left = n->right;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        x->left->parent = x;
#else
        if (x->left != nulle) {
            x->left->parent = x;
        }
#endif

        n->parent = x->parent;

        // No need to check whether x is the root because
        // this singleRightRotation function is called from
        // only doubleLeftRotation; hence, x is a right child.
        if (x == x->parent->left) {
            x->parent->left = n;
        } else {
            x->parent->right = n;
        }

        // Assign pointers for x.
        n->right = x;
        x->parent = n;

        n->color = BLACK;
        n->right->color = RED;

        if (x->left != nulle) {
            n->left->color = RED;
        }

        ++singleRotationCount;
        return n;
    }
    
    /*
     * Rotate right at a node, analogous to the LL rotation
     * of the AVL tree. Used for insertion. Assign neither
     * a parent pointer to the new root of the subtree nor
     * a pointer from the parent to that new root because
     * because those assignments cause errors in setBySide.
     * Instead, assign those pointers in setBySide. 
     * 
     * Also, do not perform some tests and assignments
     * performed by the singleRightRotation function
     * because this singleRightRotation2 function is
     * called only after singleRightRotation1 that
     * renders those tests and assignments redundant.
     * 
     * Calling parameter:
     * 
     * @param x (IN) the node at which to rotate right
     * 
     * @return the new root of the subtree created by rotation
     */
private:
    inline Node* singleRightRotation2(Node* x) {

        Node* n = x->left;
        x->left = n->right;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        x->left->parent = x;
#else
        if (x->left != nulle) {
            x->left->parent = x;
        }
#endif

        // Check whether x is the root but otherwise there is
        // no need to assign a parent pointer to n because
        // setBySide will do it.
        if (x == root) {  // equivalent to x->parent == nulle
            root = n;
            n->parent = nulle;
        }

        // Assign pointers for x.
        n->right = x;
        x->parent = n;

        // The colors of n and n->left have already been
        // assigned by the singleRightRotation1 function.
        n->right->color = RED;

        ++singleRotationCount;
        return n;
    }
    
private:
    inline Node* doubleRightRotation(Node* x) {
        if (x->left != nulle) {
            x->left = singleLeftRotation1(x->left);
            setColor(x->left->right, BLACK);
            // singleRotationCount is incremented twice
            // for each increment of doubleRotationCount.
            ++doubleRotationCount;
            return singleRightRotation2(x);
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
        if (x->right != nulle) {
            x->right = singleRightRotation1(x->right);
            setColor(x->right->left, BLACK);
            // singleRotationCount is incremented twice
            // for each increment of doubleRotationCount.
            ++doubleRotationCount;
            return singleLeftRotation2(x);
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

    /*
     * Find a node to erase from the tree but don't delete it
     * because the fixErasure function will delete it.
     * 
     * Calling parameters:
     * 
     * @param node (IN) the root of the subtree at this level of recursion
     * @param key (IN) the key to erase
     * 
     * @return upon success, return a pointer to the node that contains the key
     *         upon failure, return nulle
     */
private:
    inline Node* erase(Node* const node, K const& key) {

        // Search iteratively for the key.
        Node* ptr = node;
        while ( ptr != nulle ) {
            if ( key < ptr->key ) {
                ptr = ptr->left;
            } else if ( key > ptr->key ) {
                ptr = ptr->right;
            } else {
                // Found the key. Does the node have one child or fewer?
                if (ptr->left == nulle || ptr->right == nulle) {
                    return ptr; // Yes, so return the node.
                }

                // No, the node has two children, so replace it
                // by the leftmost node of the right subtree.
		        {
		            Node* const successor = eraseMinValue(ptr->right);
                    ptr->key = successor->key;
                    ptr = successor;
	            }
		        return ptr;
	        }
        }

        // Didn't find the key, so return nul.
        return nulle;
    }

    /*
     * Find the node that contains the minimum key.
     *
     * Calling parameters:
     * 
     * node (IN) the root of the subtree at this level of recursion
     * 
     * @return pointer to the node that contains the minimum key
     */
private:
    inline Node* eraseMinValue(Node* node) {

        // Find the leftmost node and return it.
        while (node->left != nulle) {
            node = node->left;
        }
        return node;
    }

    /*
     * Erase a key from the tree and fix the tree after erasure.
     *
     * Calling parameter:
     * 
     * @param key (IN) the key to erase
     * 
     * @return true if the key was found; otherwise, false
     */
public:
    inline bool erase(K const& key) {

        Node* node = erase(root, key);
        if (node == nulle) {
            // No need to repair the tree because it hasn't changed.
            return false;
       }

        // Repair the tree and put the node back on the freed list.
        --count;
        fixErasure(node);
        return true;
    }

    /*
     * Repair the red-black tree after erasure of a node
     * and then delete the node from the tree.
     *
     * The six different cases and their associated rules
     * for repair are discussed at:
     * 
     * https://medium.com/analytics-vidhya/deletion-in-red-black-rb-tree-92301e1474ea
     * https://www.youtube.com/watch?v=w5cvkTXY0vQ
     * 
     * Calling Parameter:
     * 
     * node (IN) the node that has been erased from the tree
     *
     * WARNING: Declaring this function inline produces a compiler warning
     *          which suggests that inlining it will cause code bloat that
     *          will decrease performance.
     */
private:
    void fixErasure(Node* const node) {

        if (node == nulle) {
            return;
        }

        // Rule 2: if the root has a single child, replace it with that child;
        // otherwise, if the root has no children, delete it.
        if (node == root) {
            if (root->left == nulle && root->right == nulle) {
                root = nulle;
                deleteNode(node);
            } else if (root->left == nulle) {
                root = root->right;
                root->color = BLACK;  // No need to call setColor because root->right exists.
                root->parent = nulle;
                deleteNode(node);
            } else {
                root = root->left;
                root->color = BLACK;  // No need to call setColor because root->left exists.
                root->parent = nulle;
                deleteNode(node);
            }
            return;
        }

        // Replace a RED node with its child, which must be BLACK. Or remove a RED leaf (i.e., Rule 1).
        // It is always possible to remove a RED node because only the number of BLACK nodes along
        // each path to the leaves is constrained. The number of RED nodes along a path is unimportant.
        // Also, because node exists, there is no need to call getColor(node).
        if (node->color == RED || getColor(node->left) == RED || getColor(node->right) == RED) {
            // child is either non-nulle node->left or non-nulle node->right or nulle node->right
            Node* child = node->left != nulle ? node->left : node->right;

            if (node == node->parent->left) {
                node->parent->left = child;
#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
                child->parent = node->parent;
                child->color = BLACK;
#else
                if (child != nulle) {
                    child->parent = node->parent;
                    child->color = BLACK;
                }
#endif
                deleteNode(node);
            } else {
                node->parent->right = child;
#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
                child->parent = node->parent;
                child->color = BLACK;
#else
                if (child != nulle) {
                    child->parent = node->parent;
                    child->color = BLACK;
                }
#endif
               deleteNode(node);
            }
        } else {
            // The node is BLACK
            Node* sibling = nulle;
            Node* parent = nulle;
            Node* ptr = node;
            ptr->color = DOUBLE_BLACK; // DB is a shorthand for ptr below; ptr exists, so no need for setColor.
            // ptr can't retreat to the root, so no need for getColor(ptr).
            while (ptr != root && ptr->color == DOUBLE_BLACK) {//
                parent = ptr->parent;
                if (ptr == parent->left) {
                    // DB is the left child of its parent.
                    sibling = parent->right;
                    if (getColor(sibling) == RED) {
                        // Rule 4: DOUBLE_BLACK's sibling is RED.
                        sibling->color = BLACK;  // sibling is RED and hence exists, so no need for setColor(sibling, BLACK).
                        parent->color = RED;  // ptr can't retreat to the root, so no need for getColor(parent).
                        rotateLeft(parent);
                    } else {
                        if (getColor(sibling->left) == BLACK && getColor(sibling->right) == BLACK) {
                            // Rule 3: DB's sibling and the sibling's children are BLACK,
                            // but what about the "null DB" comment at the following URL?
                            // https://medium.com/analytics-vidhya/deletion-in-red-black-rb-tree-92301e1474ea
                            // So, remove DOUBLE_BLACK from DB and transfer it to DB's parent and change
                            // the sibling's color to RED.
                            ptr->color = BLACK;  // ptr can't retreat to the root, so no need for setColor(ptr).
                            setColor(sibling, RED);
                            // ptr can't retreat to the root, so no need for getColor(parent) or setColor(parent, *).
                            if (parent->color == RED) {
                                parent->color = BLACK;
                            } else {
                                parent->color = DOUBLE_BLACK;
                            }
                            ptr = parent;  // Here is where ptr retreats to its parent.
                        } else if (getColor(sibling->right) == BLACK) {
                            // Rule 5: DB's sibling (i.e., right) is BLACK, DB's sibling's child that is far
                            // from DB (i.e., right) is BLACK, and DB's sibling's child that is near to DB
                            // (i.e., left) is RED. So, swap sibling's color with sibling's RED child and
                            // rotate the sibling away from DB (i.e., to the right). Do not transfer the
                            // DOUBLE_BLACK away from DB. Proceed directly to Rule 6 without iterating.
                            //
                            // Because sibling's near child is RED, both it and sibling exist, so there is
                            // no need for setColor(sibling->left, BLACK) and setColor(sibling, RED).
                            sibling->left->color = BLACK;
                            sibling->color = RED;
                            rotateRight(sibling);
                            sibling = parent->right;
                            // Here is Rule 6, without requiring another iteration of this while loop.
                            //
                            // Because sibling's far child is RED, both it and sibling exist, so there is
                            // no need for setColor(sibling->right, BLACK) or setColor(sibling, parent->color).
                            // Also, ptr can't retreat to the root, so there is no need for setColor(ptr, BLACK)
                            // or setColor(parent, BLACK).
                            sibling->color = parent->color;
                            parent->color = BLACK;
                            sibling->right->color = BLACK;
                            rotateLeft(parent);
                            ptr->color = BLACK; // Remove DOUBLE_BLACK but don't transfer it to another node.
                            break;
                        } else {
                            // Rule 6: DB's sibling (i.e., right) is BLACK and DB's sibling's child
                            // that is far from DB (i.e., right) is RED. So, swap DB's parent's
                            // color with DB's sibling's color (BLACK), rotate the parent towards
                            // DB (i.e., left), set the color of DB's sibling's far child (i.e., right)
                            // to BLACK, and change DB's color to BLACK without transferring the
                            // DOUBLE_BLACK to another node. The resulting elimination of a DB node
                            // terminates execution of the while loop.
                            //
                            // Because sibling's far child is RED, both it and sibling exist, so there is
                            // no need for setColor(sibling->right, BLACK) or setColor(sibling, parent->color).
                            // Also, ptr can't retreat to the root, so there is no need for setColor(ptr, BLACK)
                            // or setColor(parent, BLACK).
                            sibling->color = parent->color;
                            parent->color = BLACK;
                            sibling->right->color = BLACK;
                            rotateLeft(parent);
                            ptr->color = BLACK; // Remove DOUBLE_BLACK but don't transfer it to another node.
                            break;
                        }
                    }
                } else {
                    // DB is the right child of its parent.
                    sibling = parent->left;
                    if (getColor(sibling) == RED) {
                        // Rule 4: DOUBLE_BLACK's sibling is RED.
                        sibling->color = BLACK;  // sibling is RED and hence exists, so no need for setColor(sibling, BLACK).
                        parent->color = RED;  // ptr can't retreat to the root, so no need for getColor(parent).
                       rotateRight(parent);
                } else {
                        if (getColor(sibling->left) == BLACK && getColor(sibling->right) == BLACK) {
                            // Rule 3: DB's sibling and the sibling's children are BLACK,
                            // but what about the "null DB" comment at the following URL?
                            // https://medium.com/analytics-vidhya/deletion-in-red-black-rb-tree-92301e1474ea
                            // So, remove DOUBLE_BLACK from DB and transfer it to DB's parent and change
                            // the sibling's color to RED.
                            ptr->color = BLACK;  // ptr can't retreat to the root, so no need for setColor(ptr).
                            setColor(sibling, RED);
                            // ptr can't retreat to the root, so no need for getColor(parent) or setColor(parent, *).
                            if (parent->color == RED) {
                                parent->color = BLACK;
                            } else {
                                parent->color = DOUBLE_BLACK;
                            }
                            ptr = parent;  // Here is where ptr retreats to its parent.
                        } else if (getColor(sibling->left) == BLACK) {
                            // Rule 5: DB's sibling (i.e., left) is BLACK, DB's sibling's child that is far
                            // from DB (i.e., left) is BLACK, and DB's sibling's child that is near to DB
                            // (i.e., right) is RED. So, swap sibling's color with sibling's RED child and
                            // rotate the sibling away from DB (i.e., to the left). Do not transfer the
                            // DOUBLE_BLACK away from DB. Proceed directly to Rule 6 without iterating.
                            //
                            // Because sibling's near child is RED, both it and sibling exist, so there is
                            // no need for setColor(sibling->left, BLACK) and setColor(sibling, RED).
                            sibling->right->color = BLACK;
                            sibling->color = RED;
                            rotateLeft(sibling);
                            sibling = parent->left;
                            // Here is Rule 6, without requiring another iteration of this while loop.
                            //
                            // Because sibling's far child is RED, both it and sibling exist, so there is
                            // no need for setColor(sibling->right, BLACK) or setColor(sibling, parent->color).
                            // Also, ptr can't retreat to the root, so there is no need for setColor(ptr, BLACK)
                            // or setColor(parent, BLACK).
                            sibling->color = parent->color;
                            parent->color = BLACK;
                            sibling->left->color = BLACK;
                            rotateRight(parent);
                            ptr->color = BLACK; // Remove DOUBLE_BLACK but don't transfer it to another node.
                            break;
                        } else {
                            // Rule 6: DB's sibling (i.e., left) is BLACK and DB's sibling's child
                            // that is far from DB (i.e., left) is RED. So, swap DB's parent's
                            // color with DB's sibling's color (BLACK), rotate the parent towards
                            // DB (i.e., right), set the color of DB's sibling's far child (i.e., left)
                            // to BLACK, and change DB's color to BLACK without transferring the
                            // DOUBLE_BLACK to another node. The resulting elimination of a DB node
                            // terminates execution of the while loop.
                            //
                            // Because sibling's far child is RED, both it and sibling exist, so there is
                            // no need for setColor(sibling->right, BLACK) or setColor(sibling, parent->color).
                            // Also, ptr can't retreat to the root, so there is no need for setColor(ptr, BLACK)
                            // or setColor(parent, BLACK).
                            sibling->color = parent->color;
                            parent->color = BLACK;
                            sibling->left->color = BLACK;
                            rotateRight(parent);
                            ptr->color = BLACK; // Remove DOUBLE_BLACK but don't transfer it to another node.
                            break;
                        }
                    }
                }
            }

            // This node is not the root because Rule 2 has been applied above.
            // Set to nulle the parent's pointer to the node and delete the node.
            if (node == node->parent->left) {
                node->parent->left = nulle;
            } else {
                node->parent->right = nulle;
            }
            deleteNode(node);

            // The root exists and it is always BLACK.
            root->color = BLACK;
        }
    }

    /*
     * Rotate left at a node, analogous to the RR rotation
     * of the AVL tree.
     * 
     * Calling parameter:
     * 
     * @param node (IN) the node at which to rotate left
     */
private:
    inline void rotateLeft(Node* const node) {

        Node* right_child = node->right;
        node->right = right_child->left;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        node->right->parent = node;
#else
        if (node->right != nulle) {
            node->right->parent = node;
        }
#endif
        right_child->parent = node->parent;

        if (node == root) {  // equivalent to node->parent == nulle
            root = right_child;
        } else if (node == node->parent->left) {
            node->parent->left = right_child;
        } else {
            node->parent->right = right_child;
        }

        right_child->left = node;
        node->parent = right_child;

        // Increment the rotation count.
        ++rotateL;
    }

    /*
     * Rotate right at a node, analogous to the LL rotation
     * of the AVL tree.
     * 
     * Calling parameter:
     * 
     * @param node (IN) the node at which to rotate right
     */
private:
    inline void rotateRight(Node* const node) {

        Node* left_child = node->left;
        node->left = left_child->right;

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        node->left->parent = node;
#else
        if (node->left != nulle) {
            node->left->parent = node;
        }
#endif
        left_child->parent = node->parent;

        if (node == root) {  // equivalent to node->parent == nulle
            root = left_child;
        } else if (node == node->parent->left) {
            node->parent->left = left_child;
        } else {
            node->parent->right = left_child;
        }

        left_child->right = node;
        node->parent = left_child;

        // Increment the rotation count.
        ++rotateR;
    }

    /*
     * Check the tree for correctness, i.e.,
     * (1) no DOUBLE_BLACK nodes
     * (2) no RED child of a RED parent
     * (3) correct sorted order of keys
     * (4) a constant number of BLACK nodes along
     *     any path to the bottom of the tree
     * (5) except for the root, a valid parent pointer
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
        if ( node->left != nulle && node->left->key >= node->key ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " left child ";
            streamNode(node->left, buffer);
            buffer << std::endl;
            throw std::runtime_error(buffer.str());
        }
        if ( node->right != nulle && node->right->key <= node->key ) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " right child ";
            streamNode(node->right, buffer);
            buffer << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // Check for a valid parent pointer.
        if ( node != root ) { 
            if ( node->parent == nulle ) {
                std::ostringstream buffer;
                buffer << std::endl << std::endl << "node ";
                streamNode(node, buffer);
                buffer << " has invalid parent pointer" << std::endl;
                throw std::runtime_error(buffer.str());
            }
            if ( node != node->parent->left && node != node->parent->right ) {
                std::ostringstream buffer;
                buffer << std::endl << std::endl << "node ";
                streamNode(node, buffer);
                buffer << " has parent ";
                streamNode(node->parent, buffer);
                buffer << " but is neither its parent's left child ";
                streamNode(node->parent->left, buffer);
                buffer << " nor its parent's right child ";
                streamNode(node->parent->right, buffer);
                buffer << std::endl;
                throw std::runtime_error(buffer.str());
            }
        }

        // If the bottom of the tree has been reached compare counts
        // but only if the previous count is valid (i.e., non-zero).
        if ( node->left == nulle && node->right == nulle ) {
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
        if ( node->left != nulle ) {
            leftCount = checkTree( node->left, prevCount, currCount );
        }
        if ( node->right != nulle ) {
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

        if (root == nulle) {
            return 0;
        }

        // Check the color of the root node.
        if (getColor(root) != BLACK) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "root ";
            streamNode(root, buffer);
            buffer << " is not BLACK\n" << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // Check that the root node's parent is nul.
        if (root->parent != nulle) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "root's parent ";
            streamNode(root->parent, buffer);
            buffer << " is not nul" << std::endl;
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
        if (node != nulle) {
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
        if (node != nulle) {
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
        if ( p->right != nulle ) {
            printTree( p->right, d+1 );
        }

        for ( int i = 0; i < d; ++i ) {
            std::cout << "          ";
        }
        printNode(p);
        std::cout << " (";
        if (p == root) {
            std::cout << "x";
        } else {
            printNode(p->parent);
        }
        std::cout << ")" << std::endl;

        if ( p->left != nulle ) {
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

        if ( p->left != nulle ) {
            getKeys( p->left, v, i );
        }
        v[i++] = p->key;
        if ( p->right != nulle ) { 
            getKeys( p->right, v, i );
        }
    }

    /*
     * Return the color of a node.
     *
     * Calling parameter:
     * 
     * node (IN) pointer to a node
     * 
     * @return the color of the node,
     *         or BLACK is the pointer is null
     */
private:
    inline color_t getColor(Node* const node) {
        return (node == nulle) ? BLACK : node->color;
    }

    /*
     * Set the color of a node if its pointer is non-null.
     *
     * Calling parameters:
     * 
     * @param node (IN) pointer to the node
     * @param color (IN) the color to set
     */
private:
    inline void setColor(Node* const node, color_t const color) {

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        node->color = color;
#else
        if (node != nulle) {
            node->color = color;
        }
#endif
    }

    /*
     * Print the keys stored in the tree, where the key of
     * the root of the tree is at the left and the keys of
     * the leaf nodes are at the right.
     */
public:
    void printTree() {
        if ( root != nulle ) {
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
        if ( root != nulle ) {
            size_t i = 0;
            getKeys( root, v, i );
        }
    }

#undef nulle // Restrict the scope of nulle to this hyrbTree class.
};

#endif // LAKEMPER_ANANDA_HYBRID_RB_TREE_H
