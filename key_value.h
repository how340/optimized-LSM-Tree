// This file defines the basic data structure containg the key value pair
// and declares some additional useful typedef and definitions for the whole project
#pragma once
#ifndef KEY_VALUE_H
#define KEY_VALUE_H

#include <math.h>
// These values provide upper and lower bounds of key/value pairs that is supported by our system. 
typedef int32_t KEY_t;
typedef int32_t VALUE_t;

// TODO: need to consider the signed vs unsigned storage spaces. That might be a toggleable choice for later implementation. 
#define MAX_KEY 2147483647
#define MIN_KEY -2147483648

#define MAX_VAL 2147483647
#define MIN_VAL -2147483647
#define TOMBSTONE_VAL -2147483648

// basic int32 key/value pair data structure.
struct Entry {
    KEY_t key;
    VALUE_t val;

    bool operator==(const Entry& other) const {return key == other.key;}
    bool operator<(const Entry& other) const {return key < other.key;}
    bool operator>(const Entry& other) const {return key > other.key;}
};

typedef struct Entry Entry_t;

#endif