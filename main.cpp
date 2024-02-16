#include "Tree.h"
#include <iostream>

int main() {
    Tree tree;

    // Inserting values into the tree
    tree.insert(5);
    tree.insert(3);
    tree.insert(7);
    tree.insert(2);
    tree.insert(4);
    tree.insert(6);
    tree.insert(8);

    // Inorder traversal of the tree
    std::cout << "Inorder Traversal: ";
    tree.inorderTraversal();

    return 0;
}
