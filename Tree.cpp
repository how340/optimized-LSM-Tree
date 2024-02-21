#include <iostream>
#include <vector>
#include "key_value.h"
using namespace std;

// BTree node
class BTreeNode {
    //use predefined struct for key/value storage
    KEY_t* keys; 
    VALUE_t* vals;

    int t;    // order of tree (max number of child nodes)
    int n;    // Current number of key/value pairs
    
    BTreeNode **C; // An array of child pointers
    
    bool leaf; // Is true when node is leaf. Otherwise false

public:
    BTreeNode(int order, bool _leaf); // Constructor
    ~BTreeNode(); //destructor

    bool insert(int k);

    // A utility function to split the child y of this node. i is index of y in child array C[]
    void splitChild(int i, BTreeNode *y);

    // A function to traverse all nodes in a subtree rooted with this node
    void traverse();

    // A function to search a key in the subtree rooted with this node
    BTreeNode* search(int k); // returns NULL if k is not present

};

BTreeNode::BTreeNode(int order, bool _leaf){
    // copy given parameters
    t = order;
    leaf = _leaf;
    n = 0;
    // instantiate the key/value array and the child array. 
    keys = new KEY_t[t-1];
    vals = new VALUE_t[t-1];

    C = new BTreeNode *[t];
};

BTreeNode::~BTreeNode(){
     // Delete all child nodes if this is not a leaf node
    if (!leaf) {
        for (int i = 0; i <= n; ++i) {
            delete C[i];
        }
    }

    // Free the allocated memory for keys and child pointers
    delete[] keys;
    delete[] vals;
    delete[] C;
}

void BTreeNode::traverse() {
    // There are n keys and n+1 children, traverse through n keys and first n children
    int i;
    for (i = 0; i < n; i++) {
        // If this is not leaf, then before printing key[i], traverse the subtree rooted with child C[i].
        if (!leaf){
            C[i]->traverse();
        }
        cout << keys[i] << endl;
    }

    // Print the subtree rooted with last child
    if (!leaf) C[i]->traverse();
}

// TODO: Check if I need to also maintain the values as well. 
// maybe this will need modification too? 
void BTreeNode::splitChild(int i, BTreeNode *y){
// Create a new node which is going to store (t-1) keys of y
    BTreeNode *z = new BTreeNode(y->t, y->leaf);
    z->n = t - 1;

    // Copy the last (t-1) keys of y to z
    for (int j = 0; j < t-1; j++) z->vals[j] = y->vals[j+t];
    // now copy the values
    for (int j = 0; j < t-1; j++) z->vals[j] = y->vals[j+t];

    // Copy the last t children of y to z
    if (!y->leaf) {
        for (int j = 0; j < t; j++) z->C[j] = y->C[j+t];
    }

    // Reduce the number of keys in y
    y->n = t - 1;

    // Since this node is going to have a new child, create space of new child
    for (int j = n; j >= i+1; j--) C[j+1] = C[j];

    // Link the new child to this node
    C[i+1] = z;

    // A key of y will move to this node. Find the location of new key and move all greater keys one space ahead
    for (int j = n-1; j >= i; j--) keys[j+1] = keys[j];

    // Copy the middle key of y to this node
    keys[i] = y->keys[t-1];

    // Increment count of keys in this node
    n = n + 1;
};

// TODO: need to make sure this insert works correctly. 
bool BTreeNode::insert(KEY_t key, VALUE_t val) {

    // Initialize index as index of rightmost element
    int i = n-1;

    // If this is a leaf node
    if (leaf) {
        // The following loop does two things:
        // a) Finds the location of new key to be inserted
        // b) Moves all greater keys to one place ahead
        while (i >= 0 && keys[i] > k) {
            keys[i+1] = keys[i];
            i--;
        }

        // Insert the new key at found location
        keys[i+1] = k;
        n = n+1;
    } else { // If this node is not leaf
        // Find the child which is going to have the new key
        while (i >= 0 && keys[i] > k) i--;

        // See if the found child is full
        if (C[i+1]->n == 2*t-1) {
            // If the child is full, then split it
            splitChild(i+1, C[i+1]);

            // After split, the middle key of C[i] goes up and C[i] is split into two. See which of the two is going to have the new key
            if (keys[i+1] < k) i++;
        }
        C[i+1]->insert(k);
    }
};


// TODO: test b tree node implementation. 
// int main(){
//     BTreeNode t(5, false);
//     t.insert(10);
//     t.insert(20);
//     t.insert(5);
//     t.insert(6);
//     t.insert(12);
//     t.insert(30);
//     t.insert(7);
//     t.insert(17);

//     cout << "Traversal of the constructed tree is ";
//     t.traverse();
// }