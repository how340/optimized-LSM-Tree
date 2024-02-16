#ifndef TREE_H
#define TREE_H

// TreeNode class declaration
class TreeNode {
public:
    int value;
    TreeNode* left;
    TreeNode* right;

    // Constructor
    TreeNode(int val);
};

// Tree class declaration
class Tree {
public:
    TreeNode* root;

    // Constructor
    Tree();

    // Public member functions
    void insert(int val);
    void inorderTraversal();

private:
    // Private member functions
    void insertPrivate(int val, TreeNode*& node);
    void inorderTraversalPrivate(TreeNode* node);
};

#endif // TREE_H
