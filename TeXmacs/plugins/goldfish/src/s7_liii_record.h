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

#ifndef S7_LIII_RECORD_H
#define S7_LIII_RECORD_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_define_record_type (s7_scheme* sc, s7_pointer args);

void glue_liii_record (s7_scheme* sc);

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_RECORD_H */
