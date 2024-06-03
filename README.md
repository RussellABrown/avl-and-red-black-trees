The avlTree.cpp source-code file is a C++ implementation of an AVL tree. The avlMap.cpp source-code file is a C++ implementation of a key-to-value map based on an AVL tree.

These implementations are similar to the Pascal search procedure and the Pascal delete, balance1, balance2, and del procedures from pages 220-221 and pages 223-225 respectively of Nicklaus Wirth's textbook, "Algorithms + Data Structures = Programs" (Prentice-Hall, 1976) with the following two differences. (1) The bug in the del procedure (i.e., a missing 'q := r;' statement) has been fixed. (2) When it is necessary to delete a node that has two children, instead of always replacing that node with the rightmost node of the left subtree (as performed in the del procedure), that node is replaced by either the rightmost node of the left subtree or the leftmost node of the right subtree, as determined by the balance of the replaced node in an attempt to minimize subsequent rebalancing. If the balance equals +1, the right subtree is deeper than the left subtree, so the replacement node is chosen from the right subtree. But if the balance equals -1, the left subtree is deeper than the right subtree, so the replacement node is chose from the left subtree. And if the balance equals 0, the two subtrees have equal depth, so it is possible to choose the replacement node as either the rightmost node of the left subtree or the leftmost node of the right subtree, so the rightmost node of the left subtree is chosen arbitrarily.

The key type in avlTree.cpp and the key and value types in avlMap.cpp are specified via templates.

Each source-code file includes a main function that tests the AVL tree or map. The words.txt file is a dictionary of words used to test avlMap.cpp.
