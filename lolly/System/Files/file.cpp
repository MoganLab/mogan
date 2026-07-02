
/******************************************************************************
* MODULE     : file.cpp
* DESCRIPTION: file handling
* COPYRIGHT  : (C) 1999  Joris van der Hoeven
                   2023  Darcy Shen
*******************************************************************************
* This software falls under the GNU general public license version 3 or later.
* It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
* in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
******************************************************************************/

#include "file.hpp"
#include "analyze.hpp"
#include "lolly/hash/uuid.hpp"
#include "string.hpp"
#include "sys_utils.hpp"

#include "tbox/tbox.h"

bool
is_local_and_single (url u) {
  string label   = u.label ();
  string protocol= u.protocol ();
  return ((label == "") || (label == "concat") || (label == "root")) &&
         (protocol == "default" || protocol == "file");
}

static string
as_local_path (url u) {
  if (is_rooted (u, "file")) {
    return as_string (reroot (u, "default"));
  }
  return as_string (u);
}

bool
is_directory (url u) {
  if (!is_local_and_single (u)) return false;

  c_string       path (as_local_path (u));
  tb_file_info_t info;
  if (tb_file_info (path, &info)) {
    switch (info.type) {
    case TB_FILE_TYPE_DIRECTORY:
      return true;
    default:
      return false;
    }
  }
  else {
    return false;
  }
}

bool
is_regular (url u) {
  if (!is_local_and_single (u)) return false;

  c_string       path (as_local_path (u));
  tb_file_info_t info;
  if (tb_file_info (path, &info)) {
    switch (info.type) {
    case TB_FILE_TYPE_FILE:
      return true;
    default:
      return false;
    }
  }
  else {
    return false;
  }
}

bool
is_symbolic_link (url u) {
  if (!is_local_and_single (u)) return false;

  c_string       path (as_local_path (u));
  tb_file_info_t info;
  if (tb_file_info (path, &info)) {
    return (info.flags & TB_FILE_FLAG_LINK) != 0;
  }
  else {
    return false;
  }
}

bool
is_newer (url which, url than) {
  if (!is_local_and_single (which)) return false;
  if (!is_local_and_single (than)) return false;

  tb_file_info_t info1, info2;
  if (tb_file_info (c_string (as_local_path (which)), &info1) &
      tb_file_info (c_string (as_local_path (than)), &info2)) {
    return info1.mtime > info2.mtime;
  }
  else {
    return false;
  }
}

bool
is_of_type (url name, string filter) {
  if (is_ramdisc (name)) return true;

  if (!is_local_and_single (name)) return false;

  if (filter == "") return true;
  int i, n= N (filter);

  // Normal files
  if (os_win () || os_mingw ()) {
    string suf;
    if (filter == "x") {
      suf= suffix (name);
      if ((suf != "exe") && (suf != "bat") && (suf != "com")) {
        name= glue (name, ".exe");
        suf = "exe";
      }
    }
  }
  c_string       path (as_local_path (name));
  tb_file_info_t info;
  if (!tb_file_info (path, &info)) {
    return false;
  }
  for (i= 0; i < n; i++)
    switch (filter[i]) {
      // FIXME: should check user id and group id for r, w and x
    case 'f':
      if (info.type != TB_FILE_TYPE_FILE) return false;
      break;
    case 'd':
      if (info.type != TB_FILE_TYPE_DIRECTORY) return false;
      break;
    case 'l':
      if (info.flags != TB_FILE_FLAG_LINK) return false;
      break;
    case 'r':
      if (!tb_file_access (path, TB_FILE_MODE_RO)) return false;
      break;
    case 'w':
      if (!tb_file_access (path, TB_FILE_MODE_WO)) return false;
      break;
    case 'x':
      if (!tb_file_access (path, TB_FILE_MODE_EXEC)) return false;
      break;
    }
  return true;
}

int
file_size (url u) {
  if (!is_local_and_single (u)) return -1;

  c_string       path (as_local_path (u));
  tb_file_info_t info;
  if (tb_file_info (path, &info)) {
    return info.size;
  }
  else {
    return -1;
  }
}

int
last_modified (url u) {
  if (!is_local_and_single (u)) return -1;

  c_string       path (as_local_path (u));
  tb_file_info_t info;
  if (tb_file_info (path, &info)) {
    return info.mtime;
  }
  else {
    return -1;
  }
}

static tb_long_t
tb_directory_walk_func (tb_char_t const* path, tb_file_info_t const* info,
                        tb_cpointer_t priv) {
  // check
  tb_assert_and_check_return_val (path && info, TB_DIRECTORY_WALK_CODE_END);

  array<string>* p_arr_result= (array<string>*) priv;
  (*p_arr_result) << as_string (tail (url_system (string (path))));
  return TB_DIRECTORY_WALK_CODE_CONTINUE;
}

array<string>
read_directory (url u, bool& error_flag) {
  if (!is_local_and_single (u)) {
    error_flag= false;
    return array<string> ();
  }

  c_string      path (as_local_path (u));
  array<string> arr_result= array<string> ();
  error_flag              = !is_directory (u);
  if (error_flag) {
    return arr_result;
  }
  tb_directory_walk (path, 0, tb_false, tb_directory_walk_func, &arr_result);
  return arr_result;
}

url
subdirectories (url u) {
  if (is_or (u)) return subdirectories (u[1]) | subdirectories (u[2]);
  else if (is_directory (u)) {
    url           ret= u;
    bool          error_flag;
    array<string> dir= read_directory (u, error_flag);
    for (int i= 0; i < N (dir); i++)
      if (!starts (dir[i], ".") && is_directory (u * dir[i]))
        ret= ret | subdirectories (u * dir[i]);
    return ret;
  }
  else return url_none ();
}

void
mkdir (url u) {
  string label= u.label ();
  if (label == "none" || label == "root" || label == "wildcard") return;
  if (is_local_and_single (u)) { // label == "" or label == "concat"
    c_string path (as_local_path (u));
    tb_directory_create (path);
  }
  if (is_or (u)) { // label == "or"
    mkdir (u[1]);
    mkdir (u[2]);
  }
}

void
make_dir (url which) {
  if (is_none (which)) return;
  if (!is_directory (which)) {
    make_dir (head (which));
    mkdir (which);
  }
}

void
rmdir (url u) {
  string label= u.label ();
  if (label == "none" || label == "root" || label == "wildcard") return;
  if (is_local_and_single (u)) { // label == "" or label == "concat"
    c_string path (as_local_path (u));
    tb_directory_remove (path);
  }
  if (is_or (u)) { // label == "or"
    rmdir (u[1]);
    rmdir (u[2]);
  }
}

void
chdir (url u) {
  if (is_local_and_single (u)) {
    c_string path (as_local_path (u));
    if (tb_directory_current_set (path) != tb_true) {
      TM_FAILED ("Failed to change the dir");
    }
  }
  else TM_FAILED ("file path invalid");
}

url
url_temp (string suffix) {
  string file_name= replace (lolly::hash::uuid_make (), "-", "_");
  if (!is_empty (suffix)) {
    file_name= file_name * string (".") * suffix;
  }
  url u= url_temp_dir () * url (file_name);
  if (file_size (u) == -1) {
    return u;
  }
  else {
    return url_temp (suffix);
  }
}

url
url_temp_dir_sub () {
  string user= get_user_name ();
  url    base;
#if defined(OS_WIN) || defined(OS_MINGW)
  base= url_system ("$TMP");
#else
  base= url_system ("/tmp");
#endif
  static url tmp_dir=
      base * url (user) * url (".lolly") * url (as_string (get_process_id ()));
  return (tmp_dir);
}

url
url_temp_dir () {
  static url u;
  if (u == url_none ()) {
    u= url_temp_dir_sub ();
    make_dir (u);
  }
  return u;
}

void
move (url u1, url u2) {
  c_string p1 (as_local_path (u1));
  c_string p2 (as_local_path (u2));

  tb_file_rename (p1, p2);
}

void
copy (url u1, url u2) {
  c_string p1 (as_local_path (u1));
  c_string p2 (as_local_path (u2));

  tb_file_copy (p1, p2, TB_FILE_COPY_LINK);
}

void
remove (url u) {
  string label= u.label ();
  if (label == "none" || label == "root" || label == "wildcard") return;
  if (is_local_and_single (u)) {
    c_string path (as_local_path (u));
    tb_file_remove (path);
  }
  else if (is_or (u)) { // label == "or"
    remove (u[1]);
    remove (u[2]);
  }
}

/******************************************************************************
 * New style loading and saving
 *   Use two function to emulate using syntax and try-finally statement in C#
 ******************************************************************************/

struct file_status {
  bool             failed;
  const char*      error_msg;
  const char*      path;
  const tb_byte_t* buffer;
  file_status (bool failed_, const char* msg= "", const char* path_= NULL,
               const tb_byte_t* buffer_= NULL)
      : failed (failed_), error_msg (msg), path (path_), buffer (buffer_) {}
};

/**
 * \brief lookup the first exist path in a bunch of url,
 * if all url does not exist, then the last one will be used.
 */
static url
find_the_first_exist (const url& u) {
  url u_iter= expand (u);
  // iterate to find the first existed file
  while (is_or (u_iter)) {
    if (is_regular (u_iter[1])) {
      return u_iter[1];
    }
    u_iter= u_iter[2];
  }
  // if u_target does not exist, just return the last url
  return u_iter;
}

static bool
cleanup_and_return_finally (const file_status& status, const url& u, bool fatal,
                            const string& reason) {

  if (status.buffer != NULL) {
    tm_delete_array (status.buffer);
  }
  if (status.path != NULL) {
    tm_delete_array (status.path);
  }

  if (!status.failed) {
    return false;
  }
  cerr << "Failed to " << reason << " in [" << as_local_path (u) << "]" << LF;
  if (fatal) {
    TM_FAILED (status.error_msg);
  }
  return true;
}

file_status
load_string_try (url u, string& s) {
  if (!is_local_and_single (u)) {
    return file_status (true, "Must be a local and single file");
  }
  url         u_target= find_the_first_exist (u);
  const char* path    = as_charp (as_local_path (u_target));
  if (!tb_file_access (path, TB_FILE_MODE_RO)) {
    return file_status (true, "File is not readable", path);
  }
  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return file_status (true, "Failed to init the file", path);
  }
  tb_file_sync (file); // lock file
  tb_size_t size= tb_file_size (file);
  if (size == 0) {
    s= "";
    tb_file_exit (file);
    return file_status (false, "", path);
  }
  tb_byte_t* buffer     = tm_new_array<tb_byte_t> (size);
  tb_size_t  real_size  = tb_file_read (file, buffer, size);
  bool       read_sz_equ= (real_size == size);
  bool       exit_suc   = tb_file_exit (file); // exit file
  if (read_sz_equ && exit_suc) {
    s->resize (size);
    // Copying char by char has roughly the same efficiency as constructing new
    // string with buffer.
    for (size_t seek= 0; seek < size; seek++) {
      s[seek]= buffer[seek];
    }
    return file_status (false, "", path, buffer);
  }
  else {
    return file_status (true, "Unexpected behavior during reading", path,
                        buffer);
  }
}

bool
load_string (url u, string& s, bool fatal) {
  file_status stat= load_string_try (u, s);
  return cleanup_and_return_finally (stat, u, fatal, "load url");
}

string
string_load (url u) {
  string s;
  // file_url f= u;
  (void) load_string (u, s, false);
  return s;
}

file_status
save_string_try (url u, const string& s) {
  ASSERT (sizeof (tb_byte_t) == sizeof (char),
          "invalid cast from tb_byte_t* to char*");
  if (!is_local_and_single (u)) {
    return file_status (true, "Must be an absolute path");
  }
  url         u_target= find_the_first_exist (u);
  const char* path    = as_charp (as_local_path (u_target));

  // tb_file_access cannot check TB_FILE_MODE_CREAT on windows, so create
  // directly
  tb_file_ref_t fout= tb_file_init (path, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT |
                                              TB_FILE_MODE_TRUNC);
  if (fout == tb_null) {
    return file_status (true, "File not writeable", path);
  }

  // lock file
  tb_filelock_ref_t lock= tb_filelock_init (fout);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (fout);
    return file_status (true, "Fail to lock file", path);
  }

  tb_size_t        input_size= N (s);
  const tb_byte_t* content  = reinterpret_cast<const tb_byte_t*> (as_charp (s));
  tb_size_t        real_size= tb_file_writ (fout, content, input_size);
  bool             writ_sz_equ= (real_size == input_size);
  bool             release_suc= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_suc= tb_file_exit (fout);
  if (writ_sz_equ && exit_suc && release_suc) {
    return file_status (false, "", path, content);
  }
  else {
    return file_status (true, "Unexpected behavior during writting", path,
                        content);
  }
}

bool
save_string (url u, const string& s, bool fatal) {
  file_status stat= save_string_try (u, s);
  return cleanup_and_return_finally (stat, u, fatal, "save to url");
}

void
string_save (const string& s, url u) {
  (void) save_string (u, s, false);
}

file_status
append_string_try (url u, const string& s) {
  if (!is_local_and_single (u)) {
    return file_status (true, "Must be a local and single file");
  }
  url         u_target= find_the_first_exist (u);
  const char* path    = as_charp (as_local_path (u_target));

  // open the file
  tb_file_ref_t fout= tb_file_init (
      path, TB_FILE_MODE_WO | TB_FILE_MODE_APPEND | TB_FILE_MODE_CREAT);
  if (fout == NULL) {
    return file_status (true, "File to append is not found or not appendable",
                        path);
  }

  // lock file
  tb_filelock_ref_t lock= tb_filelock_init (fout);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (fout);
    return file_status (true, "Fail to lock file", path);
  }

  // append string to file
  tb_size_t        input_size= N (s);
  const tb_byte_t* content  = reinterpret_cast<const tb_byte_t*> (as_charp (s));
  tb_size_t        real_size= tb_file_writ (fout, content, input_size);
  bool             writ_sz_equ= (real_size == input_size);
  bool             release_suc= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_suc= tb_file_exit (fout);

  if (writ_sz_equ && exit_suc && release_suc) {
    return file_status (false, "", path, content);
  }
  else {
    return file_status (true, "Unexpected behavior during appending", path,
                        content);
  }
}

bool
append_string (url u, const string& s, bool fatal) {
  file_status stat= append_string_try (u, s);
  return cleanup_and_return_finally (stat, u, fatal, "append to url");
}

void
string_append_to_file (const string& s, url u) {
  (void) append_string (u, s, false);
}

void
append_to (url what, url to) {
  string what_s;
  if (load_string (what, what_s, false) || append_string (to, what_s, false))
    cerr << "Append failed for " << to << LF;
}
