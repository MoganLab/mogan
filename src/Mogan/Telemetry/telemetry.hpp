
/******************************************************************************
 * MODULE     : telemetry.hpp
 * DESCRIPTION: C++ glue for Scheme telemetry tracking
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include "scheme.hpp"
#include "string.hpp"

#if !IS_COMMUNITY

inline void
telemetry_track (string event_type) {
  call ("track-event", object (event_type),
        tmscm_to_object (eval_scheme ("'()")));
}

inline void
telemetry_track (string event_type, string props) {
  call ("track-event", object (event_type),
        tmscm_to_object (eval_scheme (props)));
}

#else

inline void
telemetry_track (string event_type) {
  (void) event_type;
}

inline void
telemetry_track (string event_type, string props) {
  (void) event_type;
  (void) props;
}

#endif // !IS_COMMUNITY

#endif // TELEMETRY_HPP
