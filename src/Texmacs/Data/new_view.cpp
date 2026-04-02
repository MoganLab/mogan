
/******************************************************************************
 * MODULE     : new_view.cpp
 * DESCRIPTION: View management
 * COPYRIGHT  : (C) 1999-2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "convert.hpp"
#include "dictionary.hpp"
#include "file.hpp"
#include "message.hpp"
#include "new_document.hpp"
#include "new_window.hpp"
#include "tm_data.hpp"
#include "tm_file.hpp"
#include "tm_link.hpp"
#include "tm_url.hpp"
#include "view_history.hpp"
#include "web_files.hpp"

#include <moebius/data/scheme.hpp>
#include <moebius/drd/drd_std.hpp>

#if defined(OS_MINGW) || defined(OS_WIN)
#define WINPATHS
#endif

using namespace moebius;
using moebius::data::scm_quote;
using moebius::drd::the_drd;

/******************************************************************************
 * Associating URLs to views
 ******************************************************************************/

static hashmap<tree, int>     view_number_table (0);
static hashmap<tree, pointer> view_table (NULL);

static int
new_view_number (url u) {
  tree key= as_tree (u);
  view_number_table (key)+= 1;
  return view_number_table[key];
}

tm_view_rep::tm_view_rep (tm_buffer buf2, editor ed2)
    : buf (buf2), ed (ed2), win (NULL), win_tabpage (NULL),
      nr (new_view_number (buf->buf->name)) {}

static string
encode_url (url u) {
  if (!is_rooted (u)) return "here/" * as_string (u, URL_UNIX);
  if (get_root (u) == "default") return "default" * as_string (u, URL_UNIX);
  return get_root (u) * "/" * as_string (unroot (u), URL_UNIX);
}

static url
decode_url (string s) {
  int i= search_forwards ("/", 0, s);
  if (i < 0) return url_none ();
#ifdef WINPATHS
  int j= 0;
  if (s (0, i) == "here") j= i + 1;
  if (s (0, i) == "default") j= i;
  if (j != 0) {
    if (s[j + 1] == ':') return url (s (j, N (s)));
    else return url_root ("default") * url (s (j, N (s)));
  }
#else
  if (s (0, i) == "here") return url (s (i + 1, N (s)));
  if (s (0, i) == "default") return url (s (i, N (s)));
#endif
  return url_root (s (0, i)) * url_general (s (i + 1, N (s)), URL_CLEAN_UNIX);
}

url
abstract_view (tm_view vw) {
  if (vw == NULL) return url_none ();
  string name= encode_url (vw->buf->buf->name);
  // cout << vw->buf->buf->name << " -> " << name << "\n";
  string nr= as_string (vw->nr);
  return url_system ("tmfs://view/" * nr * "/" * name);
}

tm_view
concrete_view (url u) {
  if (is_none (u)) return NULL;
  string s= as_string (u);
  if (!starts (s, "tmfs://view/")) return NULL;
  s    = s (N (string ("tmfs://view/")), N (s));
  int i= search_forwards ("/", 0, s);
  if (i < 0) return NULL;
  int nr  = as_int (s (0, i));
  url name= decode_url (s (i + 1, N (s)));
  // cout << s (i+1, N(s)) << " -> " << name << "\n";
  tm_buffer buf= concrete_buffer (name);
  if (!is_nil (buf))
    for (i= 0; i < N (buf->vws); i++)
      if (buf->vws[i]->nr == nr) return buf->vws[i];
  return NULL;
}

/******************************************************************************
 * Views associated to editor, window, or buffer
 ******************************************************************************/

tm_view the_view= NULL;

bool
has_current_view () {
  return the_view != NULL;
}

void
set_current_view (url u) {
  tm_view vw= concrete_view (u);
  // ASSERT (is_none (u) || starts (as_string (tail (u)), "no_name") || vw !=
  // NULL, "bad view");
  the_view= vw;
  if (vw != NULL) {
    the_drd                 = vw->ed->drd;
    vw->buf->buf->last_visit= texmacs_time ();
  }
}

url
get_current_view () {
  ASSERT (the_view != NULL, "no active view");
  return abstract_view (the_view);
}

url
get_current_view_safe () {
  if (the_view == NULL) return url_none ();
  return abstract_view (the_view);
}

void notify_delete_view (url u);

editor
get_current_editor () {
  url     u = get_current_view ();
  tm_view vw= concrete_view (u);
  if (vw == NULL) { // HACK: shouldn't happen!
    TM_FAILED ("Current view is NULL");
    notify_delete_view (u);
    array<url> history= get_all_views ();
    if (as_tree (history) == as_tree (NULL) || N (history) == 0)
      TM_FAILED ("View history is empty")
    return view_to_editor (history[N (history) - 1]);
  }
  return vw->ed;
}

array<url>
buffer_to_views (url name) {
  tm_buffer  buf= concrete_buffer (name);
  array<url> r;
  if (is_nil (buf)) return r;
  for (int i= 0; i < N (buf->vws); i++)
    r << abstract_view (buf->vws[i]);
  return r;
}

url
view_to_buffer (url u) {
  tm_view vw= concrete_view (u);
  if (vw == NULL) return url_none ();
  return vw->buf->buf->name;
}

url
view_to_window (url u) {
  tm_view vw= concrete_view (u);
  if (vw == NULL) return url_none ();
  return abstract_window (vw->win);
}

url
view_to_window_of_tabpage (url u) {
  tm_view vw= concrete_view (u);
  if (vw == NULL) return url_none ();
  if (vw->win_tabpage == NULL) return url_none ();

  // Try to get the window URL, but catch any potential memory access issues
  try {
    return abstract_window (vw->win_tabpage);
  } catch (...) {
    return url_none ();
  }
}

editor
view_to_editor (url u) {
  tm_view vw= concrete_view (u);
  if (vw == NULL) {
    notify_delete_view (u); // HACK: returns to valid (?) state.
    failed_error << "View is " << u << "\n";
    TM_FAILED ("View admits no editor");
  }
  return vw->ed;
}

int
count_tabpages_in_window (url win_u) {
  int        count= 0;
  tm_window  win  = concrete_window (win_u);
  array<url> vws  = get_all_views ();
  int        vws_N= N (vws);
  for (int i= 0; i < vws_N; i++) {
    tm_view vwi= concrete_view (vws[i]);
    if (vwi != NULL && vwi->win_tabpage == win) {
      count++;
    }
  }
  return count;
}

/******************************************************************************
 * Viewing history
 ******************************************************************************/

static view_history view_history_instance;

void
notify_set_view (url u) {
  view_history_instance.add_view (u);
}

void
notify_delete_view (url u) {
  view_history_instance.remove_view (u);
}

url
get_recent_view (url name, bool same, bool other, bool active, bool passive) {
  // Get most recent view with the following filters:
  //   If same, then the name of the buffer much be name
  //   If other, then the name of the buffer much be other than name
  //   If active, then the buffer must be active
  //   If passive, then the buffer must be passive
  int i;
  for (i= 0; i < view_history_instance.size (); i++) {
    tm_view vw= concrete_view (view_history_instance.get_view_at_index (i));
    if (vw != NULL) {
      if (same && vw->buf->buf->name != name) continue;
      if (other && vw->buf->buf->name == name) continue;
      if (active && vw->win == NULL) continue;
      if (passive && vw->win != NULL) continue;
      return view_history_instance.get_view_at_index (i);
    }
  }
  return url_none ();
}

array<url>
get_all_views () {
  return view_history_instance.get_all_views ();
}

array<url>
get_all_views_unsorted (bool current_window_only) {
  url window= current_window_only ? get_current_window () : url_none ();
  view_history::FilteredView filtered=
      view_history_instance.get_views_for_window (window, true);
  return filtered.views;
}

void
move_tabpage (int old_pos, int new_pos) {
  url cur_win= get_current_window ();

  // 获取当前窗口的过滤和排序的view
  view_history::FilteredView filtered=
      view_history_instance.get_views_for_window (cur_win, true);
  int filtered_views_N= N (filtered.views);

  // 先验证操作的位置是否合法
  if (old_pos < 0 || old_pos >= filtered_views_N || new_pos < 0 ||
      new_pos >= filtered_views_N) {
    if (DEBUG_AUTO) {
      debug_automatic << "Invalid indices for swapping tabpages: " << old_pos
                      << ", " << new_pos << LF;
    }
    return;
  }

  // 进行移动操作
  // 先记录新老位置的原始排序依据索引，因为其会在过程中被覆盖
  int target_number= filtered.numbers[new_pos];
  int old_number   = filtered.numbers[old_pos];
  if (new_pos > old_pos) {
    for (int i= 0; i < filtered_views_N; i++) {
      if (filtered.numbers[i] <= target_number &&
          filtered.numbers[i] > old_number) {
        int original_idx= filtered.original_indices[i];
        view_history_instance.dec_number_at_index (original_idx);
      }
    }
  }
  else if (new_pos < old_pos) {
    for (int i= 0; i < filtered_views_N; i++) {
      if (filtered.numbers[i] >= target_number &&
          filtered.numbers[i] < old_number) {
        int original_idx= filtered.original_indices[i];
        view_history_instance.inc_number_at_index (original_idx);
      }
    }
  }

  // 最后，为移动的视图设置新的排序依据编号
  view_history_instance.set_number_at_index (filtered.original_indices[old_pos],
                                             target_number);
}

/******************************************************************************
 * Creation of views on buffers
 ******************************************************************************/

url tm_init_buffer_file= url_none ();
url my_init_buffer_file= url_none ();

url
get_new_view (url name) {
  // cout << "Creating new view " << name << "\n";

  create_buffer (name, tree (DOCUMENT));
  tm_buffer buf= concrete_buffer (name);
  editor    ed = new_editor (get_server ()->get_server (), buf);
  tm_view   vw = tm_new<tm_view_rep> (buf, ed);
  buf->vws << vw;
  ed->set_data (buf->data);

  url temp= get_current_view_safe ();
  set_current_view (abstract_view (vw));
  if (is_none (tm_init_buffer_file))
    tm_init_buffer_file= "$TEXMACS_PATH/progs/init-buffer.scm";
  if (is_none (my_init_buffer_file))
    my_init_buffer_file= "$TEXMACS_HOME_PATH/progs/my-init-buffer.scm";
  if (exists (tm_init_buffer_file))
    exec_file (materialize (tm_init_buffer_file));
  if (exists (my_init_buffer_file))
    exec_file (materialize (my_init_buffer_file));
  set_current_view (temp);

  // cout << "View created " << abstract_view (vw) << "\n";
  return abstract_view (vw);
}

url
get_passive_view (url name) {
  // Get a view on a buffer, but not one which is attached to a window
  // Create a new view if no such view exists
  tm_buffer buf= concrete_buffer_insist (name);
  if (is_nil (buf)) return url_none ();
  array<url> vs= buffer_to_views (name);
  for (int i= 0; i < N (vs); i++) {
    url win= view_to_window (vs[i]);
    if (is_none (win)) return vs[i];
  }
  return get_new_view (buf->buf->name);
}

url
get_passive_view_of_tabpage (url name) {
  // Get a view on a buffer, but not one which is related to a tabpage
  // Create a new view if no such view exists
  tm_buffer buf= concrete_buffer_insist (name);
  if (is_nil (buf)) return url_none ();
  array<url> vs     = buffer_to_views (name);
  int        vs_N   = N (vs);
  url        cur_win= has_current_view () && has_current_window ()
                          ? get_current_window ()
                          : url_none ();
  for (int i= 0; i < vs_N; i++) {
    url win        = view_to_window (vs[i]);
    url win_tabpage= view_to_window_of_tabpage (vs[i]);
    if (is_none (win_tabpage)) return vs[i];
    if (!is_none (cur_win) && win_tabpage == cur_win) return vs[i];
    if (is_none (win) && win_tabpage == cur_win) return vs[i];
  }
  return get_new_view (buf->buf->name);
}

url
get_recent_view (url name) {
  // Get (most) recent view on a buffer, with a preference for
  // the current buffer or another view attached to a window
  array<url> vs= buffer_to_views (name);
  if (N (vs) == 0) return get_new_view (name);
  url u= get_current_view ();
  if (view_to_buffer (u) == name) return u;
  url r= get_recent_view (name, true, false, true, false);
  if (!is_none (r)) return r;
  r= get_recent_view (name, true, false, false, false);
  if (!is_none (r)) return r;
  return vs[0];
}

/******************************************************************************
 * Destroying a view
 ******************************************************************************/

void
delete_view (url u) {
  tm_view vw= concrete_view (u);
  if (vw == NULL) return;
  tm_buffer buf= vw->buf;
  int       i, j, n= N (buf->vws);
  for (i= 0; i < n; i++)
    if (buf->vws[i] == vw) {
      array<tm_view> a (n - 1);
      for (j= 0; j < n - 1; j++)
        if (j < i) a[j]= buf->vws[j];
        else a[j]= buf->vws[j + 1];
      buf->vws= a;
    }
  notify_delete_view (u);
  vw->ed->buf= NULL;
  tm_delete (vw);
}

void
kill_tabpage (url win_u, url u) {
  // 此方法用于更精确地关闭一个标签页。
  // 虽然已有关闭视图的相关方法，但这些方法未考虑到当前引入的 tabpage 概念。
  // 随着 tabpage 的出现，用户关闭文档的操作实际上演变为关闭整个标签页。
  // 因此，本方法以 tabpage 为单位执行关闭操作，实现了标签页关闭的完整逻辑
  // 在关闭文档，或标签页时都将调用此方法。
  tm_view vw= concrete_view (u);
  if (vw == NULL) return;
  if (vw->buf != NULL && vw->buf->buf->name == url ("tmfs://startup-tab")) {
    return;
  }
  tm_window win        = vw->win;
  tm_window win_tabpage= vw->win_tabpage;
  if (win_tabpage == NULL) return;
  if (win == NULL) win= win_tabpage;
  bool is_current= (get_current_view () == u);

  // 第一步: 设定 win_tabpage
  // 将 win_tabpage 设为空指针，因为要关闭tabpage了
  vw->win_tabpage= NULL;

  // 第二步: Detach routine
  // 如果是当前视图，则需要将其从窗口中分离
  // 参照 detach_view 方法
  if (is_current) {
    vw->win   = NULL;
    widget wid= win_tabpage->wid;
    vw->ed->suspend ();
    set_scrollable (wid, glue_widget ());
    // 不需要设置窗口名称和 URL，因为将要 attach 或者关闭窗口
  }

  // 第三步: Attach routine
  // 如果是当前视图（也就是分离了当前视图），则需要附着一个新的视图
  // 新的视图是 get_all_views 循环找到的中第一个处于本标签栏的视图，
  // 也就是 view_history 中的上一个视图
  // 如果没找到可用的视图，则 found 保持为 false, 后续将关闭窗口
  // 将在下一个事件循环刷新显示
  bool       found= false;
  array<url> vws  = get_all_views ();
  if (is_current) {
    for (int i= 0; i < N (vws); i++) {
      tm_view vw2= concrete_view (vws[i]);
      if (vw2 != NULL && vw2 != vw && vw2->win_tabpage == win_tabpage) {
        window_set_view (win_u, vws[i], true);
        found= true;
        break;
      }
    }
  }

  // 第四步: Window killing
  // 如果关闭的视图是当前视图，并且没有找到其他视图附着到窗口上，
  // 则当前窗口没有可用视图，需要关闭窗口
  if (!found && is_current) {
    // if cannot find a view, close the window
    kill_window (win_u);
  }

  // 第五步: Buffer & View deleting
  // 在最后释放视图和缓冲区的资源
  tm_buffer  buf    = vw->buf;
  array<url> buf_vws= buffer_to_views (buf->buf->name);
  if (N (buf_vws) == 1) {
    // 如果缓冲区只有一个视图，则释放缓冲区和视图
    int nr, bufs_N= N (bufs);
    for (nr= 0; nr < bufs_N; nr++) {
      if (bufs[nr] == buf) {
        delete_view (u);
        // 如果将要释放的是最后一个缓冲区，则退出整个程序
        if (bufs_N == 1 && number_of_servers () == 0) get_server ()->quit ();
        // 维护缓冲区数组，释放缓冲区
        for (int i= nr; i < bufs_N - 1; i++) {
          bufs[i]= bufs[i + 1];
        }
        bufs->resize (bufs_N - 1);
        tm_delete (buf);
        break;
      }
    }
  }
  else {
    // 如果缓冲区有多个视图，则只释放当前视图，不释放缓冲区
    delete_view (u);
  }
}

void
notify_rename_before (url old_name) {
  array<url> vs= buffer_to_views (old_name);
  for (int i= 0; i < N (vs); i++)
    notify_delete_view (vs[i]);
}

void
notify_rename_after (url new_name) {
  array<url> vs= buffer_to_views (new_name);
  for (int i= 0; i < N (vs); i++)
    notify_set_view (vs[i]);
}

/******************************************************************************
 * Attaching and detaching views
 ******************************************************************************/

void
attach_view (url win_u, url u) {
  tm_window win= concrete_window (win_u);
  tm_view   vw = concrete_view (u);
  if (win == NULL || vw == NULL) return;
  // cout << "Attach view " << vw->buf->buf->name << "\n";
  vw->win= win;
  if (vw->win_tabpage == NULL) {
    vw->win_tabpage= win;
  }
  widget wid= win->wid;
  set_scrollable (wid, vw->ed);
  vw->ed->cvw= wid.rep;
  ASSERT (is_attached (wid), "widget should be attached");
  // 先通知 view 被设置，确保 view_history 更新后再调用 resume
  // 这样 resume() 中的菜单刷新能获取到正确的 view 列表
  notify_set_view (u);
  vw->ed->resume ();
  win->set_window_name (vw->buf->buf->title);
  // set_window_url 移到 window_set_view 中，在 set_current_view 之后调用
  // win->set_window_url (vw->buf->buf->name);
  // cout << "View attached\n";
}

void
detach_view (url u) {
  tm_view vw= concrete_view (u);
  if (vw == NULL) return;
  tm_window win= vw->win;
  if (win == NULL) return;
  // cout << "Detach view " << vw->buf->buf->name << "\n";
  vw->win   = NULL;
  widget wid= win->wid;
  ASSERT (is_attached (wid), "widget should be attached");
  vw->ed->suspend ();
  set_scrollable (wid, glue_widget ());
  win->set_window_name ("TeXmacs");
  win->set_window_url (url_none ());
  // cout << "View detached\n";
}

/******************************************************************************
 * Switching views
 ******************************************************************************/

void
window_set_view (url win_u, url new_u, bool focus) {
  // cout << "set view " << win_u << ", " << new_u << ", " << focus << "\n";
  tm_window win= concrete_window (win_u);
  if (win == NULL) return;
  // cout << "Found window\n";
  tm_view new_vw= concrete_view (new_u);
  if (new_vw == NULL || new_vw->win == win) return;
  // cout << "Found view\n";
  ASSERT (new_vw->win == NULL, "view attached to other window");
  url old_u= window_to_view (win_u);
  if (!is_none (old_u)) detach_view (old_u);
  attach_view (win_u, new_u);
  if (focus || get_current_view () == old_u) set_current_view (new_u);
  // 在 set_current_view 之后调用 set_window_url，确保 SLOT_FILE 处理时 current
  // view 已更新
  win->set_window_url (new_vw->buf->buf->name);
  exec_delayed (scheme_cmd ("(make-cursor-visible '" *
                            scm_quote (as_string (new_u)) * ")"));
  exec_delayed (scheme_cmd ("(when (defined? 'refresh-auxiliary-widget) "
                            "(refresh-auxiliary-widget))"));
}

void
view_set_new_window (url view_u) {
  if (view_u == url_none ()) return;
  tm_view view              = concrete_view (view_u);
  url     view_win_tabpage_u= abstract_window (view->win_tabpage);
  if (count_tabpages_in_window (view_win_tabpage_u) <= 1) {
    // 窗口中只有这一个标签页，无需操作
    return;
  }
  url       win_u   = new_window ();
  tm_window win     = concrete_window (win_u);
  bool      attached= (view->win != nullptr);
  if (attached) {
    switch_to_other_tabpage (view_u);
  }
  detach_view (view_u);
  attach_view (win_u, view_u);
  view->win_tabpage= win;
  hack_refresh_window_editors (view_win_tabpage_u, win_u, false);
}

bool
view_set_window (url view_u, url win_u, bool focus) {
  url     current_view_u    = get_current_view ();
  tm_view view              = concrete_view (view_u);
  url     view_win_tabpage_u= abstract_window (view->win_tabpage);
  if (win_u == url_none () || view == nullptr || view_win_tabpage_u == win_u) {
    return false;
  }
  tm_window win     = concrete_window (win_u);
  bool      found   = false;
  bool      attached= (view->win != nullptr);
  if (attached) {
    found= switch_to_other_tabpage (view_u);
  }
  detach_view (view_u);
  if (focus) {
    window_set_view (win_u, view_u, false);
    set_visibility (win->wid, true);
  }
  else {
    // 安全检查：确保 win_tabpage 仍然有效
    if (view->win_tabpage != nullptr) {
      set_visibility (view->win_tabpage->wid, true);
    }
  }
  view->win_tabpage= win;
  if (attached && !found) {
    // view 所在的 TabBar 没有其他标签页了
    kill_window (view_win_tabpage_u);
    return true;
  }
  else {
    hack_refresh_window_editors (view_win_tabpage_u, win_u, (!focus));
  }
  return false;
}

void
hack_refresh_window_editors (url win1, url win2, bool revert) {
  // HACK: rusume 各自的 editor，触发标签栏 UI 的刷新

  // 安全检查：确保窗口仍然有效
  tm_window win1_obj= concrete_window (win1);
  tm_window win2_obj= concrete_window (win2);

  if (win1_obj == nullptr || win2_obj == nullptr) {
    return;
  }

  url view1= window_to_view (win1);
  url view2= window_to_view (win2);

  tm_view vw1= concrete_view (view1);
  tm_view vw2= concrete_view (view2);

  // 安全检查：确保视图不为空
  if (vw1 == nullptr || vw2 == nullptr) {
    return;
  }

  editor ed1= vw1->ed;
  editor ed2= vw2->ed;
  if (revert) {
    editor tmp= ed1;
    ed1       = ed2;
    ed2       = tmp;
  }
  ed1->resume ();
  ed1->suspend ();
  ed2->resume ();
}

bool
switch_to_other_tabpage (url view_u) {
  tm_view    view = concrete_view (view_u);
  array<url> vws  = get_all_views ();
  int        vws_N= N (vws);
  for (int i= 0; i < vws_N; i++) {
    tm_view vwi= concrete_view (vws[i]);
    if (vwi != NULL && vwi != view && vwi->win_tabpage == view->win_tabpage) {
      window_set_view (abstract_window (view->win), vws[i], true);
      return true;
    }
  }
  return false;
}

void
switch_to_buffer (url name) {
  // cout << "Switching to buffer " << name << "\n";
  url     u = get_passive_view_of_tabpage (name);
  tm_view vw= concrete_view (u);
  if (vw == NULL) return;
  window_set_view (get_current_window (), u, true);
  tm_window nwin= vw->win;
  if (nwin != NULL)
    nwin->set_window_zoom_factor (nwin->get_window_zoom_factor ());
  // cout << "Switched to buffer " << vw->buf->buf->name << "\n";
}

void
set_current_drd (url name) {
  url     u = get_passive_view (name);
  tm_view vw= concrete_view (u);
  if (vw != NULL) the_drd= vw->ed->drd;
}

void
focus_on_editor (editor ed) {
  array<url> bufs= get_all_buffers ();
  for (int i= 0; i < N (bufs); i++) {
    array<url> vs= buffer_to_views (bufs[i]);
    for (int j= 0; j < N (vs); j++)
      if (concrete_view (vs[j]) != NULL && view_to_editor (vs[j]) == ed) {
        set_current_view (vs[j]);
        return;
      }
  }

  /* FIXME: directly using get_all_views produces synchronization error
  array<url> vs= get_all_views ();
  for (int i=0; i<N(vs); i++)
    if (view_to_editor (vs[i]) == ed) {
      cout << "Focus on " << vs[i] << "\n";
      set_current_view (vs[i]);
      return;
    }
  */

  std_warning << "Warning: editor no longer exists, "
              << "may indicate synchronization error\n";
  // failed_error << "Name of buffer: " << ed->buf->buf->name << "\n";
  // TM_FAILED ("invalid situation");
}

/**
 * @brief 聚焦于指定的缓冲区。
 *
 * 尝试将当前视图切换到与指定缓冲区名称对应的视图。
 * 查找视图的优先级逻辑如下：
 * 1. 检查当前视图是否已经是目标缓冲区。
 * 2. 查找该缓冲区最近使用的视图（优先查找已激活/显示在窗口中的视图）。
 * 3. 如果未找到最近视图（即处于非激活状态），则查找任意存在的视图。
 * 4.
 * 如果仍未找到（例如该缓冲区没有关联任何视图），则尝试获取该缓冲区的第一个可用视图列表中的视图。
 *
 * 如果找到目标视图，将其设置为当前视图 (set_current_view)。
 *
 * @param name    目标缓冲区的 URL 名称。
 * @param isfocus 是否将键盘焦点（Keyboard Focus）发送到该视图的编辑器。
 * 如果为 true，则调用 send_keyboard_focus。
 * @return bool   如果成功找到并切换到该缓冲区的视图，则返回 true；
 * 如果该缓冲区没有任何关联视图，则返回 false。
 */
bool
focus_on_buffer (url name, bool isfocus) {
  // Focus on the most recent view on a buffer, preferably active in a window
  // Return false if no view exists for the buffer
  if (the_view != NULL && the_view->buf->buf->name == name) return true;
  url r= get_recent_view (name, true, false, true, false);
  if (is_none (r)) r= get_recent_view (name, true, false, false, false);
  if (is_none (r)) {
    array<url> vws= buffer_to_views (name);
    if (N (vws) > 0) r= vws[0];
  }
  if (is_none (r)) return false;
  tm_view new_vw= concrete_view (r);
  if (isfocus) send_keyboard_focus (new_vw->ed);
  set_current_view (r);
  return true;
}

bool
var_focus_on_buffer (url name) {
  if (the_view != NULL && the_view->buf->buf->name == name) return true;
  url r= get_recent_view (name, true, false, true, false);
  if (is_none (r)) r= get_recent_view (name, true, false, false, false);
  if (is_none (r)) {
    array<url> vws= buffer_to_views (name);
    if (N (vws) > 0) r= vws[0];
  }
  if (is_none (r)) return false;
  the_view->ed->suspend ();
  set_current_view (r);
  tm_view new_vw= concrete_view (r);
  new_vw->ed->resume ();
  send_keyboard_focus (new_vw->ed);
  return true;
}

void
make_cursor_visible (url u) {
  // Make the cursor visible in the view
  tm_view vw= concrete_view (u);
  if (vw == NULL) return;
  tm_window win= vw->win;
  if (win == NULL) return;
  editor ed= vw->ed;
  if (ed == NULL) return;
  ed->make_cursor_visible ();
}

url
get_most_recent_view () {
  if (view_history_instance.size () < 1) {
    return url_none ();
  }
  return view_history_instance.get_view_at_index (0);
}

void
invalidate_most_recent_view () {
  url vw= get_most_recent_view ();
  if (is_none (vw)) return;
  concrete_view (vw)->ed->typeset_invalidate_all ();
}

bool
is_tmfs_view_type (string s, string type) {
  // 检查前缀
  string prefix  = "tmfs://view/";
  int    s_N     = N (s);
  int    prefix_N= N (prefix);
  if (s_N <= prefix_N) return false;
  if (!(s (0, prefix_N) == prefix)) return false;

  // 查找 “/数字”
  int i= prefix_N;
  while (i < s_N && s[i] >= '0' && s[i] <= '9')
    i++;
  if (i == prefix_N) return false; // 没有数字

  // 中间匹配 type
  string mid;
  if (type != "default") {
    mid= "/tmfs/" * type * "/";
  }
  else {
    mid= "/default/";
  }
  int mid_N= N (mid);
  if (s_N < i + mid_N) return false;
  if (!(s (i, i + mid_N) == mid)) return false;

  // 后面可以是任意字符串
  return true;
}

bool
is_tmfs_view_type (url u, string type) {
  return is_tmfs_view_type (as_string (u), type);
}
