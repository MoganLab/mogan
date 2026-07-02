#include "tree_patch.hpp"
#include "tree_observer.hpp"

void
apply (patch p, tree& t) {
  switch (get_type (p)) {
  case PATCH_MODIFICATION:
    apply (t, get_modification (p));
    break;
  case PATCH_BRANCH:
    ASSERT (N (p) <= 1, "ambiguous application");
  case PATCH_COMPOUND:
  case PATCH_AUTHOR:
    for (int i= 0; i < N (p); i++)
      apply (p[i], t);
    break;
  case PATCH_BIRTH:
    break;
  default:
    TM_FAILED ("unsupported patch type");
  }
}
