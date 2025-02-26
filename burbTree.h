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
 * The following code is a modification of Rao Ananda's C++ implementation
 * of a bottom-up red-black tree found at the following Github URl.
 *
 * https://github.com/anandarao/Red-Black-Tree
 * 
 * The modification removes bugs and eliminates memory leaks
 * from the fixDeleteRBT function, which is renamed to fixErasure.
 * 
 * The modification also optionally replaces recursion with
 * iteration in the insert and erase functions.
 * 
 * Compile with a test program, for example, burbTree.cpp via:
 * 
 * g++ -std=c++11 -O3 test_burbTree.cpp
 * 
 * To use recursion instead of iteration, compile via:
 * 
 * g++ -std=c++11 -O3 -D RECURSION test_burbTree.cpp
 * 
 * To enable selection of a preferred replacement node
 * when a 2-child node is deleted, compile via:
 * 
 * g++ -std=c++11 -O3 -D ENABLE_PREFERRED_TEST test_burbTree.cpp
 *
 * To enable selection of a preferred replacment node
 * but force selection of the in-order successor (to
 * assess the overhead relative to no selection of a
 * preferred replacement node), compile via:
 *
 * g++ -std=c++11 -O3 -D ENABLE_PREFERRED_TEST -D FORCE_SUCCESSOR test_burbTree.cpp
 * 
 * To invert selection of a preferred replacement node
 * when the balance of the 2-child node is 0, compile via:
 * 
 * g++ -std=c++11 -O3 -D ENABLE_PREFERRED_TEST -D INVERT_PREFERRED_TEST test_burbTree.cpp
 *
 * To disable the freed list that avoids re-use of new and delete, compile via:
 * 
 * g++ -std=c++11 -O3 -D DISABLE_FREED_LIST test_burbTree.cpp
 * 
 * To preallocate the freed list as a vector of red-black tree nodes, compile via:
 * 
 * g++ -std=c++11 -O3 -D PREALLOCATE test_burbTree.cpp
 * 
 * To use a non-static sentinel node nullnode instead of nullptr, compile via:
 * 
 * g++ -std=c++11 -O3 -D NULL_NODE test_burbTree.cpp
 * 
 * To use a static sentinel node nullnode instead of nullptr,
 * NOTE that C++17 is required and compile via:
 * 
 * g++ -std=c++17 -O3 -D STATIC_NULL_NODE test_burbTree.cpp
 */

#ifndef BAYER_GUIBAS_SEDGEWICK_ANANDA_BU_RB_TREE_H
#define BAYER_GUIBAS_SEDGEWICK_ANANDA_BU_RB_TREE_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

/*
 * The rbTree class defines the root of the hybrid red-black tree
 * and provides the RED, BLACK, and DOUBLE_BLACK uint8_t constants.
 */
template <typename K>
class burbTree
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

    // Don't rely on the compiler to use int for an enum.
private:
    typedef uint8_t color_t;
    static constexpr color_t RED = 0;
    static constexpr color_t BLACK = 1;
    static constexpr color_t DOUBLE_BLACK = 2;

    /* The Node struct defines a node in the BURB tree. */
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

#ifdef ENABLE_PREFERRED_TEST
        size_t taille; // the number of nodes in the subtree
#endif

public:
        Node() {
            color = RED;
            left = right = parent = nullptr;
            
#ifdef ENABLE_PREFERRED_TEST
            taille = 1;
#endif
        }

public:
        Node(K key) {
            this->key = key;
            color = RED;
            left = right = parent = nullptr;
            
#ifdef ENABLE_PREFERRED_TEST
            taille = 1;
#endif
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
    Node* root;     // the root of the tree
    size_t count;   // the number of nodes in the tree

#ifndef DISABLE_FREED_LIST
    Node* freed;    // the freed list
#ifdef PREALLOCATE
    std::vector<Node> nodes;
#endif
#endif

public:
    size_t rotateL, rotateR; // rotation counters

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
    burbTree() {

#if defined(NULL_NODE) && !defined(STATIC_NULL_NODE)
        nulle = new Node();
#endif
        root = nulle;
        count = rotateL = rotateR = 0;

#ifndef DISABLE_FREED_LIST
        freed = nulle;
#endif
    }
    
public:
    ~burbTree() {
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
     * Delete every node in the BURB subtree.  If the tree has been
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
     * Delete every node in the BURB tree.  If the tree has been
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
#ifndef PREALLOCATE
        while ( freed != nulle ) {
            Node* next = freed->left;
            delete freed;
            freed = next;
        }
#else
        nodes.clear();
#endif
        freed = nulle;
#endif
    }

    /* Report the number of nodes on the freed list. */
public:
    size_t freedSize() {
        size_t count = 0;
#ifndef DISABLE_FREED_LIST
        Node* ptr = freed;
        while (ptr != nulle) {
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
            Node* p = createNode();
            p->left = freed;
            freed = p;
        }
#else
        nodes.resize(n);
        for (size_t i = 0; i < n; ++i) {
            Node* p = &nodes[i];
#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
            // The Node() constructor does not recognize nullnode
            // when nullnode is defined as a Node pointer. See
            // the createNode functions.
            p->left = p->right = p->parent = nulle;
#endif
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
            
#ifdef ENABLE_PREFERRED_TEST
            ptr->taille = 1;
#endif
            return ptr;
        } else
#endif
        {
            return createNode(key);
        }
    }

#ifdef RECURSION
    /*
     * Search the tree for the existence of a key.
     * If the key is not found, the node that contains
     * it is added to the tree and then the tree is
     * rebalanced if necessary. If the key is found,
     * the node is ignored.
     *
     * Calling parameters:
     *
     * @param node (IN) the root of the subtree at this level of recursion
     * @oaram parent (IN) the parent of the root of the subtree
     * @param ptr (IN) a node that contains the key to add to the tree
     * @param inserted (MODIFIED) returns true upon successful insertion;
     *                            otherwise returns false
     * 
     * @return upon successful insertion, return the node that contains the key;
     *         upon non-insertion, return the root node of the subtree
     */
private:
    Node* insert(Node* const node, Node* const parent, Node* const ptr, bool& inserted) {

        if (node == nulle) {
            // insert the node because it isn't already in the tree
            ptr->parent = parent;
            inserted = true;
            return ptr;
        }

        // Increment the size of the inserted node's parent so that
        // the new size will propagate upward as recursion unwinds.
        if (ptr->key < node->key) {
            node->left = insert(node->left, node, ptr, inserted);
#ifdef ENABLE_PREFERRED_TEST
            if (inserted == true) {
                node->taille++;  // node exists, so no need for incSize(node).
            }
#endif
        } else if (ptr->key > node->key) {
            node->right = insert(node->right, node, ptr, inserted);
#ifdef ENABLE_PREFERRED_TEST
            if (inserted == true) {
                node->taille++;  // node exists, so no need for incSize(node).
            }
#endif
        } else {
            // For a tree, don't insert the key twice.
            // For a map, overwrite the value.
        }

        return node;
    }

    /*
     * Search the tree for the existence of a key.
     * If the key is not found, it is is added to
     * the tree as a new node and then the tree is
     * rebalanced if necessary. If the key is found,
     * it is ignored.
     *
     * Calling parameter:
     *
     * @param key (IN) the key to add to the tree
     * 
     * @return true if the key was added as a new node; otherwise, false
     */
public:
    inline bool insert(K const& key) {
        Node* node = newNode(key);
        bool result, inserted = false;
        root = insert(root, nullptr, node, inserted);
        if (inserted == false) {
            // The node wasn't inserted, so put it back on freed list.
            deleteNode(node);
            result = false;
        } else {
            // The node was inserted, so fix the tree.
            fixInsertion(node);
            ++count;
            result = true;
        }
        return result;
    }
#else   // RECURSION

    /*
     * Search the tree for the existence of a key.
     * If the key is not found, the node that contains
     * it is added to the tree and then the tree is
     * rebalanced if necessary. If the key is found,
     * the node is ignored.
     *
     * Calling parameter:
     *
     * @param node (IN) a node that contains the key to add to the tree
     * 
     * @return true if the key was added as a new node; otherwise, false
     */
private:
    inline bool insert(Node* const node) {

        // Search iteratively for the point of insertion.
        Node* ptr = root;
        Node* parent = nulle;
        while ( ptr != nulle ) {
            if ( node->key < ptr->key ) {
                parent = ptr;
                ptr = ptr->left;
            } else if ( node->key > ptr->key ) {
                parent = ptr;
                ptr = ptr->right;
            } else {
                // Found the key.
                // For a tree, don't insert the key twice.
                // For a map, overwrite the value.
                return false;
            }
        }

        // Didn't find the key, so insert the new node.
        if (node->key < parent->key) {
            parent->left = node;
        } else {
            parent->right = node;
        }
        node->parent = parent;

        // Increment the size of each subtree along the path
        // that retreats to the root.
#ifdef ENABLE_PREFERRED_TEST
        ptr = node->parent;
        while (ptr != nulle) {
            // node is not the root, so parent exists and hence no need for incSize(ptr).
            ptr->taille++;
            ptr = ptr->parent;
        }
#endif
        return true;
    }

    /*
     * Search the tree for the existence of a key.
     * If the key is not found, it is is added to
     * the tree as a new node and then the tree
     * is rebalanced if necessary. If the key is
     * found, it is ignored.
     *
     * Calling parameter:
     *
     * @param key (IN) the key to add to the tree
     * 
     * @return true if the key was added as a new node; otherwise, false
     */
public:
    inline bool insert(K const& key) {

        bool result = false;
        Node* node = newNode(key);
        if (root == nulle) {
            // Insertion always succeeds if the tree is empty.
            root = node;
            node->parent = nulle;
            node->color = BLACK;
            ++count;
            result = true;
        } else {
            result = insert(node);
            if (result == true) {
                // The node was inserted, so fix the tree.
                fixInsertion(node);
                ++count;
            } else {
                // The node wasn't inserted, so put it back on freed list.
                deleteNode(node);
            }
        }
        return result;
    }
#endif  // RECURSION

    /*
     * Search the tree for the existence of a key.
     *
     * Calling parameters:
     *
     * @param node (IN) the root of the subtree
     * @param key (IN) the key to search for
     * 
     * @return true if the key was found; otherwise, false
     */
private:
    inline bool contains(Node* const node, K const& key) {

        // Search iteratively for the key.
        Node* ptr = node;
        while ( ptr != nulle ) {
            if ( key < ptr->key ) {
                ptr = ptr->left;
            } else if ( key > ptr->key ) {
                ptr = ptr->right;
            } else {
                return true; // found the key
            }
        }
        return false; // didn't find the key
    }

    /*
     * Search the tree for the existence of a key.
     *
     * Calling parameter:
     *
     * @param key (IN) the key to search for
     * 
     * @return true if the key was found; otherwise, false
     */
public:
    inline bool contains(K const& key) {
        if (root == nulle) {
            return false;
        }
        return contains(root, key);
    }

#ifdef RECURSION
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
     *         upon failsure, return nullptr
     */
private:
    Node* erase(Node* const node, K const& key) {

        // The bottom of the tree has been reached.
        if (node == nulle) {
            return node;
        }

        // If node to erase has been found, decrement
        // the size of each node along the path to the
        // the root of the subtree as recursion unwinds.
        if (key < node->key) {
#ifdef ENABLE_PREFERRED_TEST
            Node* const temp = erase(node->left, key);
            if (temp != nulle) {
                node->taille--;  // node exists, so no need for decSize(node).
            }
            return temp;
#else
            return erase(node->left, key);
#endif
        }
        if (key > node->key) {
#ifdef ENABLE_PREFERRED_TEST
            Node* const temp = erase(node->right, key);
            if (temp != nulle) {
                node->taille--;  // node exists, so no need for decSize(node).
            }
            return temp;
#else
            return erase(node->right, key);
#endif
        }

        // Found the key. Does the node have one child or fewer?
        if (node->left == nulle || node->right == nulle) {
            // Yes, so return the node.
#ifdef ENABLE_PREFERRED_TEST
            node->taille--;  // node exists, so no need for decSize(node).
#endif
            return node;
        }

        // No, the node has two children, so replace it
        // either by the leftmost node of the right subtree
        // or by the rightmost node of the left subtree.
        // Select the preferred replacement node from the
        // larger of the two subtrees. That replacement
        // node will be deleted by the fixErasure function.
    	// Note: FORCE_SUCCESSOR and INVERT_PREFERRED_TEST
    	// are included only for diagnostic purposes.
#ifdef ENABLE_PREFERRED_TEST
#ifdef FORCE_SUCCESSOR
	    if (false)
#else
#ifdef INVERT_PREFERRED_TEST
        if ( getSize(node->left) > getSize(node->right) )
#else
        if ( getSize(node->left) >= getSize(node->right) )
#endif
#endif
        {
            node->taille--;  // node exists, so no need for decSize(node).
            Node* predecessor = eraseMaxValue(node->left);
            node->key = predecessor->key;
            return predecessor;
        } else
#endif
        {
#ifdef ENABLE_PREFERRED_TEST
            node->taille--;  // node exists, so no need for decSize(node).
#endif
            Node* successor = eraseMinValue(node->right);
            node->key = successor->key;
            return successor;
        }
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
            return false;
        }

        // Repair the tree and put the node back on the freed list.
        --count;
        fixErasure(node);
        return true;
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
    Node* eraseMinValue(Node* const node) {

        // Decrement the size of the node because it
        // lies along the path to the leftmost node.
#ifdef ENABLE_PREFERRED_TEST
            node->taille--;  // node exists, so no need for decSize(node).
#endif

        // Find the leftmost node and return it.
        if (node->left != nulle) {
            return eraseMinValue(node->left);
         } else {
            return node;
         }
    }

    /*
     * Find the node that contains the maximum key.
     *
     * Calling parameters:
     * 
     * node (IN) the root of the subtree at this level of recursion
     * 
     * @return pointer to the node that contains the maximum key
     */
private:
    Node* eraseMaxValue(Node* const node) {

        // Decrement the size of the node because it
        // lies along the path to the rightmost node.
#ifdef ENABLE_PREFERRED_TEST
            node->taille--;  // node exists, so no need for decSize(node).
#endif

        // Find the rightmost node and return it.
        if (node->right != nulle) {
            return eraseMaxValue(node->right);
         } else {
            return node;
         }
    }
#else  //RECURSION

    /*
     * Find a node to erase from the tree but don't delete it
     * because the fixErasure function will delete it.
     * 
     * Calling parameters:
     * 
     * @param node (IN) the root of the subtree
     * @param key (IN) the key to erase
     * 
     * @return upon success, return a pointer to the node that contains the key
     *         upon failure, return nullptr
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
                // either by the leftmost node of the right subtree
                // or by the rightmost node of the left subtree.
                // Select the preferred replacement node from the
                // larger of the two subtrees. That replacement
                // node will be deleted by the fixErasure function.
		        // Note: FORCE_SUCCESSOR and INVERT_PREFERRED_TEST
		        // are included only for diagnostic purposes.
#ifdef ENABLE_PREFERRED_TEST
#ifdef FORCE_SUCCESSOR
		        if (false)
#else
#ifdef INVERT_PREFERRED_TEST
		        if ( getSize(ptr->left) > getSize(ptr->right) )
#else
		        if ( getSize(ptr->left) >= getSize(ptr->right) )
#endif
#endif
		        {
		            Node* predecessor = eraseMaxValue(ptr->left);
                    ptr->key = predecessor->key;
                    ptr = predecessor;
	            } else
#endif
		        {
		            Node* successor = eraseMinValue(ptr->right);
                    ptr->key = successor->key;
                    ptr = successor;
	            }
		        return ptr;
	        }
        }

        // Didn't find the key, so return nullptr.
        return nullptr;
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

        // Decrement the size of each subtree along the path
        // that retreats to the root.
#ifdef ENABLE_PREFERRED_TEST
        Node* ptr = node;
        while (ptr != nulle) {
            ptr->taille--;  // ptr exists, so no need for decSize(ptr).
            ptr = ptr->parent;
        }
#endif
        // Repair the tree and put the node back on the freed list.
        --count;
        fixErasure(node);
        return true;
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
     * Find the node that contains the maximum key.
     *
     * Calling parameters:
     * 
     * node (IN) the root of the subtree at this level of recursion
     * key (MODIFIED) returns the maximum key
     * 
     * @return pointer to the node that contains the maximum key
     */
private:
    inline Node* eraseMaxValue(Node* node) {

        // Find the rightmost node and record its key.
        while (node->right != nulle) {
            node = node->right;
        }
       return node;
    }
#endif  // RECURSION

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

        // Update the node's size. The child node inherits
        // the node's prior size.
#ifdef ENABLE_PREFERRED_TEST
        right_child->taille = node->taille;
        updateSize(node);
#endif
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

        if (node == root) { // equivalent to node->parent == nulle
            root = left_child;
        } else if (node == node->parent->left) {
            node->parent->left = left_child;
        } else {
            node->parent->right = left_child;
        }

        left_child->right = node;
        node->parent = left_child;

        // Update the node's size. The child node inherits
        // the node's prior size.
#ifdef ENABLE_PREFERRED_TEST
        left_child->taille = node->taille;
        updateSize(node);
#endif
        // Increment the rotation count.
        ++rotateR;
    }

    /*
     * Repair the red-black tree after insertion.
     *
     * The three different cases and their associated rules
     * for repair are discussed at:
     * 
     * https://pages.cs.wisc.edu/~cs400/readings/Red-Black-Trees/
     * https://www.youtube.com/watch?v=qA02XWRTBdw
     * 
     * Calling Parameter:
     * 
     * node (IN) the node that has been inserted into the tree
     */
private:
    inline void fixInsertion(Node* const node) {

        Node* ptr = node;
        Node* parent = nulle;
        Node* grandparent = nulle;
        // Rule 1: repair is necessary only if the node and its parent are both RED.
        // Also, because ptr exists, there is no need to call getColor(ptr).
        while (ptr != root && ptr->color == RED && getColor(ptr->parent) == RED) {
            parent = ptr->parent;
            grandparent = parent->parent;
            if (parent == grandparent->left) {
                // The node's parent is the left child of its grandparent.
                Node* uncle = grandparent->right;
                if (getColor(uncle) == RED) {
                    // Rule 2b or4b: the node's uncle (its parent's sibling) is
                    // RED, so invert the colors of the node, its parent, and its
                    // grandparent. (However, don't invert the color of the
                    // grandparent if it is the root; this constraint is
                    // enforced only after exit from the while loop where
                    // the root color is forced to BLACK.) Because the
                    // pre-inverted color of the parent was RED, the
                    // pre-inverted color of the grandparent was BLACK and
                    // is now RED, so another iteration of the while loop
                    // is required to check the grandparent against its
                    // parent to ensure that both are not RED.
                    //
                    // Because this while loop executes only if the parent's color
                    // is RED, the parent is not the root and hence the grandparent
                    // exists, so there is no need for setColor(grandparent, RED)
                    // or setColor(parent, BLACK).
                    setColor(uncle, BLACK);
                    parent->color = BLACK;
                    grandparent->color = RED;
                    ptr = grandparent;
                } else {
                    // Rule 2a or 4a: the node's uncle (its parent's sibling) is
                    // either BLACK or non-existent (i.e., BLACK by definition).
                    // So if the node is its parent's right child, rotate
                    // left at the parent and then designate the node to be
                    // the parent and also designate the parent to be the
                    // grandparent.
                    if (ptr == parent->right) {
                        rotateLeft(parent);
                        ptr = parent;
                        parent = ptr->parent;
                    }
                    // The node is (either orignally or now) its parent's
                    // left child, so rotate right at the grandparent and
                    // then swap the colors of the parent and grandparent.
                    // Because the original colors of the node and its
                    // parent were RED, the grandparent's color is BLACK,
                    // so no further iteration of the while loop is needed.
                    rotateRight(grandparent);
                    std::swap(parent->color, grandparent->color);
                    break;
                }
            } else {
                // The node's parent is the right child of its grandparent.
                Node* uncle = grandparent->left;
                if (getColor(uncle) == RED) {
                    // Rule 2b or 4b: the node's uncle (its parent's sibling) is
                    // RED, so invert the colors of the node, its parent, and its
                    // grandparent. (However, don't invert the color of the
                    // grandparent if it is the root; this constraint is
                    // enforced only after exit from the while loop where
                    // the root color is forced to BLACK.) Because the
                    // pre-inverted color of the parent was RED, the
                    // pre-inverted color of the grandparent was BLACK and
                    // is now RED, so another iteration of the while loop
                    // is required to check the grandparent against its
                    // parent to ensure that both are not RED.
                    //
                    // Because this while loop executes only if the parent's color
                    // is RED, the parent is not the root and hence the grandparent
                    // exists, so there is no need for setColor(grandparent, RED)
                    // or setColor(parent, BLACK).
                    setColor(uncle, BLACK);
                    parent->color = BLACK;
                    grandparent->color = RED;
                    ptr = grandparent;
                } else {
                    // Rule 2a or 4a: the nodes's uncle (its parent's sibling) is
                    // either BLACK or non-existent (i.e., BLACK by definition).
                    // So if the node is its parent's left child, rotate
                    // right at the parent and then designate the node to be
                    // the parent and also designate the parent to be the
                    // grandparent.
                    if (ptr == parent->left) {
                        rotateRight(parent);
                        ptr = parent;
                        parent = ptr->parent;
                    }
                    // The node is (either orignally or now) its parent's
                    // right child, so rotate left at the grandparent and
                    // then swap the colors of the parent and grandparent.
                    // Because the original colors of the node and its
                    // parent were RED, the grandparent's color is BLACK,
                    // so no further iteration of the while loop is needed.
                    rotateLeft(grandparent);
                    std::swap(parent->color, grandparent->color);
                    break;
                }
            }
        }

        // The root exists and it is always BLACK.
        root->color = BLACK;
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
     * Check the tree for correctness, i.e.,
     * (1) no DOUBLE_BLACK nodes
     * (2) a correct size for each subtree
     * (3) no RED child of a RED parent
     * (4) correct sorted order of keys
     * (5) except for the root, a valid parent pointer 
     * (6) a constant number of BLACK nodes along
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

        // Check for a DOUBLE_BLACK.
        if ( getColor(node) == DOUBLE_BLACK ) {
            std::ostringstream buffer;
            buffer << std::endl << "node ";
            streamNode(node, buffer);
            buffer << " is DOUBLE_BLACK" << std::endl;
            throw std::runtime_error(buffer.str());
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
     * Check the BURB tree for correctness and count
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

        // Check that the root node's parent is nullptr.
        if (root->parent != nulle) {
            std::ostringstream buffer;
            buffer << std::endl << std::endl << "root's parent ";
            streamNode(root->parent, buffer);
            buffer << " is not nullptr" << std::endl;
            throw std::runtime_error(buffer.str());
        }

        // Call checkTree twice:
        // (1) to initialize prevCount without comparing black counts
        // (2) to compare prevCount to the black count at each leaf 
        size_t prevCount = 0;
        prevCount = checkTree(root, prevCount, 0);
        return checkTree(root, prevCount, 0);
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
     * Print the keys stored in the tree, where the key of
     * the root of the tree is at the left and the keys of
     * the leaf nodes are at the right.
     * 
     * Calling parameters:
     * 
     * @param p (IN) the root of the subtree at this level of recursion
     * @param d (MODIFIED) the depth in the tree
     */
public:
    void printTree() {
        if (root != nulle) {
            printTree(root, 0);
        }
    }

    /*
     * Print the key and color of a node.
     *
     * Calling parameter:
     * 
     * @param node (IN) pointer to the node
     */
private:
    void printNode(Node* const node) {
        if (node != nulle) {
            std::cout << node->key;
            if (node->color == RED) {
                std::cout << "r";
            } else if (node->color == BLACK) {
                std::cout << "b";
            } else if (node->color == DOUBLE_BLACK) {
                std::cout << "d";
            } else {
                std::cout << "?";
            }
        }
    }

    /*
     * Print the key and color of a node to a stringstream.
     *
     * Calling parameters:
     * 
     * @param node (IN) pointer to the node
     * @param buffer (MODIFIED) the stringstream
     */
private:
    void streamNode(Node* const node, std::ostringstream& buffer) {
        if (node != nulle) {
            buffer << node->key;
            if (node->color == RED) {
                buffer << "r";
            } else if (node->color == BLACK) {
                buffer << "b";
            } else if (node->color == DOUBLE_BLACK) {
                buffer << "d";
            } else {
                buffer<< "?";
            }
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

    /* Return the number of nodes in the BURB tree. */
public:
    size_t size() {
        return count;
    }

    /* Return true if there are no nodes in the BURB tree. */
public:
    bool empty() {
        return ( count == 0 );
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

    /*
     * Return the number of nodes in the subtree.
     *
     * Calling parameter:
     * 
     * @param (IN) root of the subtree at this level of recursion
     * 
     * @return the size of the subtree
     */
#ifdef ENABLE_PREFERRED_TEST
private:
    inline size_t getSize(Node* const node) {
        return (node == nulle) ? 0 : node->taille;
    }

    /*
     * Set the size of a node's subtree if its pointer is non-null.
     *
     * Calling parameters:
     * 
     * @param node (IN) pointer to the node
     * @param taille (IN) the size to set
     */
private:
    inline void setSize(Node* const node, size_t const taille) {

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        node->taille = taille;
#else
        if (node != nulle) {
            node->taille = taille;
        }
#endif
    }

    /*
     * Increment a node's size if its pointer is non-null.
     *
     * Calling parameter:
     *
     * node (IN) pointer to the node
     */
private:
    inline void incSize(Node* const node) {

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        node->taille++;
#else
        if (node != nulle) {
            node->taille++;
        }
#endif
    }

    /*
     * Decrement a node's size if its pointer is non-null.
     *
     * Calling parameter:
     *
     * node (IN) pointer to the node
     */
private:
    inline void decSize(Node* node) {

#if defined(NULL_NODE) || defined(STATIC_NULL_NODE)
        node->taille--;
#else
        if (node != nulle) {
            node->taille--;
        }
#endif
    }

    /*
     * Update a node's size from the sizes of its left
     * and right subtrees if its pointer is non-null.
     *
     * Calling parameter:
     *
     * node (IN) pointer to the node
     */
private:
    inline void updateSize(Node* const node) {
        setSize( node, getSize(node->left) + 1 + getSize(node->right) );
    }

    /*
     * Check the stored size of a node against its
     * computed size if its pointer is non-null.
     *
     * Calling parameter:
     *
     * node (IN) pointer to the node
     */
private:
    inline void checkSize(Node* const node) {
        if ( getSize(node) != getSize(node->left) + 1 + getSize(node->right) ) {
            std::ostringstream buffer;
            buffer << std::endl << "error at node ";
            streamNode(node, buffer);
            buffer << "  stored node size = " << getSize(node)
                    << "  !=   computed node size = "
                    << (getSize(node->left) + 1 + getSize(node->right)) << std::endl;
            throw std::runtime_error(buffer.str());
        }
    }
#endif // ENABLE_PREFERRED_TEST

#undef nulle // Restrict the scope of nulle to this rbTree class.
};

#endif // BAYER_GUIBAS_SEDGEWICK_ANANDA_BU_RB_TREE_H
