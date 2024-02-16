#include "levels.h"
#include <iostream>

// Constructor definition
Level::Level(int val) : value(val) {}

// Function to add a child to the node
void Level::addChild(TreeNode* child) {
    children.push_back(child);
}

// Destructor to deallocate memory used by children
Level::~TreeNode() {
    for (auto child : children) {
        delete child;
    }
}