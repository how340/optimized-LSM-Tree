#ifndef levels.h 
#define levels.h


// Each level will be implemented as a tree with n nodes, where the n = level
class Level {
    
public:
    int value;  // Value of the node
    std::vector<TreeNode*> children;  // Pointers to children nodes

    TreeNode(int val);  // Constructor
    void addChild(TreeNode* child);  // Function to add a child to the node
    ~TreeNode();  // Destructor to deallocate memory used by children
};


#endif