//
// Copyright (C) 2026 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#ifndef LIII_SIGNAL_HPP
#define LIII_SIGNAL_HPP

#include "s7.h"

namespace goldfish {

bool dispatch_pending_signals (s7_scheme* sc);
bool signal_catch_active ();
void glue_liii_signal (s7_scheme* sc);

} // namespace goldfish

#endif // LIII_SIGNAL_HPP
