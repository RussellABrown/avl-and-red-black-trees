The C++ implementation of an AVL tree achieves improved performance for AVL node deletion via the algorithm proposed by Caxton C. Foster in his 1965 article, "A Study of AVL Trees" (Goodyear Aerospace Corporation internal document GER-12158), and also described in the following pre-print article:

https://arxiv.org/abs/2406.05162

as well as in the final article that appears in Software: Practice and Experience 55(9):1607,2025:

https://doi.org/10.1002/spe.3437

The data plotted in Figures 2-8 of these articles is found in the Figures_data directory.

The AVL tree implementation (avlTree.h, test_avlTree.cpp, and testAVLTree.cpp) was transcribed from Nicklaus Wirth's Pascal implementation of the AVL tree in his 1976 textbook, "Algorithms + Data Structures = Programs." A bug in the del procedure was fixed and that procedure was bifurcated to create the eraseLeft and eraseRight functions that confer improved performance for deletion.

The bottom-up red-black tree implementation (burbTree.h, test_burbTree.cpp, and testBURBTree.cpp) was copied from Rao Ananda's C++ implementation of the bottom-up red-black tree (https://github.com/anandarao/Red-Black-Tree). The fixInsertRBTree and fixDeleteRBTree functions were renamed fixInsertion and fixErasure respectively and then optimized. Bugs and memory leaks were fixed in the fixDeleteRBTree function.

The top-down red-black tree implementation (tdrbTree.h, test_tdrbTree.cpp, and testTDRBTree.cpp) was transcribed from Cullen LaKemper's Java implementation of the top-down red-black tree (https://github.com/SangerC/TopDownRedBlackTree). A bug was fixed in the removeStep2B2 method.

The left-leaning red-black tree implementation (llrbTree.h, test_llrbTree.cpp, and testLLRBTree.cpp) was transcribed from Rene Argento's Java implementation of the left-leaning red-black tree (https://github.com/reneargento/algorithms-sedgewick-wayne/blob/master/src/chapter3/section3/RedBlackBST.java and https://github.com/reneargento/algorithms-sedgewick-wayne/blob/master/src/chapter3/section3/Exercise41_Delete.java). No bugs were detected.

The hybrid red-black tree implementation (hyrbTree.h, test_hyrbTree.cpp) uses top-down insertion and bottom-up deletion. Top-down insertion is slightly faster than bottom-up insertion. Bottom-up deletion is significantly faster than top-down deletion.

