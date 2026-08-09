
/******************************************************************************
 * MODULE     : boot.hpp
 * DESCRIPTION: manipulation of TeX font files
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef BOOT_H
#define BOOT_H

#include "tree.hpp"
#include "url.hpp"

extern tree texmacs_settings;
extern int  install_status;
extern bool headless_mode;

void init_texmacs_path (int& argc, char** argv);
void init_upgrade ();
void init_texmacs_front ();
void init_texmacs ();
void init_plugins ();
void setup_texmacs ();
void release_boot_lock ();
void init_scheme ();
void init_env_vars ();
void init_misc ();
int  load_settings_and_check_version ();

void TeXmacs_main (int argc, char** argv);

#endif // defined BOOT_H
