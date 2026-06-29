
#ifndef TREE_OBSERVER_H
#define TREE_OBSERVER_H

#include "modification.hpp"
#include <moebius/tree_label.hpp>

using moebius::tree_label;

void stretched_print (tree t, bool ips= false, int indent= 0);

void assign (path p, tree t);
void insert (path p, tree ins);
void remove (path p, int nr);
void split (path p);
void join (path p);
void assign_node (path p, tree_label op);
void insert_node (path p, tree ins);
void remove_node (path p);
void set_cursor (path p, tree data);
void touch (path p);

void assign (tree& ref, tree t);
void insert (tree& ref, int pos, tree t);
void remove (tree& ref, int pos, int nr);
void split (tree& ref, int pos, int at);
void join (tree& ref, int pos);
void assign_node (tree& ref, tree_label op);
void insert_node (tree& ref, int pos, tree t);
void remove_node (tree& ref, int pos);
void set_cursor (tree& ref, int pos, tree data);

extern bool busy_modifying;
extern bool busy_versioning;
bool        busy_tree (tree& ref);

void apply (tree& t, modification mod);

#endif // defined TREE_OBSERVER_H