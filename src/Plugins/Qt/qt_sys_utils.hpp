
/******************************************************************************
 * MODULE     : qt_sys_utils.hpp
 * DESCRIPTION: external command handling
 * COPYRIGHT  : (C) 2009-2016  David MICHEL, Denis RAUX
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_SYS_UTILS_H
#define QT_SYS_UTILS_H

#include "string.hpp"
#include "url.hpp"

string qt_get_current_cpu_arch ();
string qt_get_pretty_os_name ();
string qt_stem_user_agent ();
string qt_stem_device_id ();
void   qt_open_url (url u);

#endif // defined QT_SYS_UTILS_H
