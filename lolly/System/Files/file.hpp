
/******************************************************************************
 * MODULE     : file.hpp
 * DESCRIPTION: file handling
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2023  Darcy Shen
 *                  2023  Pluto Ye
 *                  2023  Jingkaimori
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LOLLY_FILE_H
#define LOLLY_FILE_H

#include "url.hpp"

bool is_local_and_single (url u);

bool is_directory (url u);
bool is_regular (url u);
bool is_symbolic_link (url u);
bool is_newer (url which, url than);
bool is_of_type (url name, string filter);

int file_size (url u);
int last_modified (url u);

array<string> read_directory (url u, bool& error_flag);
url           subdirectories (url u);

void mkdir (url u);
void make_dir (url which);
void rmdir (url u);
void chdir (url u);

url url_temp_dir ();
url url_temp (string suffix= "");

void remove (url u);
void move (url u1, url u2);
void copy (url u1, url u2);

bool   load_string (url file_name, string& s, bool fatal);
string string_load (url u);
bool   save_string (url file_name, const string& s, bool fatal= false);
void   string_save (const string& s, url u);
bool   append_string (url u, const string& s, bool fatal);
void   string_append_to_file (const string& s, url u);
void   append_to (url what, url to);
#endif
