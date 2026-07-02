#ifndef OBSERVERS_H
#define OBSERVERS_H

#include "observer.hpp"

class editor_rep;
class archiver_rep;

observer edit_observer (editor_rep* ed);

observer search_observer (tree& ref, int type);
bool     admits_edit_observer (tree t);

observer highlight_observer (int lan, array<int> cols);

void       remove_observer (observer& o, observer what);
void       attach_highlight (tree& ref, int lan);
void       attach_highlight (tree& ref, int lan, int col, int start, int end);
bool       has_highlight (tree& ref, int lan);
array<int> obtain_highlight (tree& ref, int lan);
void       detach_highlight (tree& ref, int lan);

observer ip_observer (path ip);

path obtain_ip (tree& ref);
void attach_ip (tree& ref, path ip);
void detach_ip (tree& ref);
bool ip_attached (path ip);

observer list_observer (observer o1, observer o2);
void     insert_observer (observer& o, observer what);
void     clean_observers (tree& ref);
void     attach_observer (tree& ref, observer o);
void     detach_observer (tree& ref, observer o);

observer undo_observer (archiver_rep* arch);

observer tree_pointer (tree t, bool flag= false);
observer scheme_observer (tree t, string cb);
tree     obtain_tree (observer o);
observer tree_pointer_new (tree t);
void     tree_pointer_delete (observer o);

observer tree_position (tree t, int index);
path     obtain_position (observer o);

observer tree_addendum_new (tree t, int kind, blackbox bb, bool keep= true);
void     tree_addendum_delete (observer o);
void     tree_addendum_delete (tree t, int type);

void edit_announce (editor_rep* ed, modification mod);
void edit_done (editor_rep* ed, modification mod);
void edit_touch (editor_rep* ed, path p);

#endif // defined OBSERVERS_H