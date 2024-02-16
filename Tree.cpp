#include "Tree.h"
#include <iostream>

// TreeNode constructor implementation
TreeNode::TreeNode(int val) : value(val), left(nullptr), right(nullptr) {}

// Tree constructor implementation
Tree::Tree() : root(nullptr) {}

// Function to insert a value into the tree
void Tree::insert(int val) {
    insertPrivate(val, root);
}

// Helper function for insertion
void Tree::insertPrivate(int val, TreeNode*& node) {
    if (node == nullptr) {
        node = new TreeNode(val);
    } else if (val < node->value) {
        insertPrivate(val, node->left);
    } else {
        insertPrivate(val, node->right);
    }
}

// Function for inorder traversal
void Tree::inorderTraversal() {
    inorderTraversalPrivate(root);
    std::cout << std::endl;
}

// Helper function for inorder traversal
void Tree::inorderTraversalPrivate(TreeNode* node) {
    if (node != nullptr) {
        inorderTraversalPrivate(node->left);
        std::cout << node->value << " ";
        inorderTraversalPrivate(node->right);
    }
}
