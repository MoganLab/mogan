/*!The Treasure Box Library
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright (C) 2009-present, TBOOX Open Source Group.
 *
 * @author      ruki
 * @file        uuid.c
 * @ingroup     hash
 *
 */

/* //////////////////////////////////////////////////////////////////////////////////////
 * includes
 */
#include "uuid.h"
#include "md5.h"
#include "../libc/libc.h"
#include "../math/math.h"
#include "../utils/utils.h"
#include "../platform/platform.h"

/* //////////////////////////////////////////////////////////////////////////////////////
 * private implementation
 */

// http://xorshift.di.unimi.it/xorshift128plus.c
static tb_uint64_t tb_uuid4_xorshift128plus(tb_uint64_t* s)
{
    tb_uint64_t s1       = s[0];
    tb_uint64_t const s0 = s[1];
    s[0] = s0;
    s1 ^= s1 << 23;
    s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return s[1] + s0;
}
static tb_bool_t tb_uuid4_generate(tb_byte_t uuid[16], tb_byte_t buffer[16])
{
    /* generate uuid
     *
     * t: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
     *    xxxxxxxxxxxx4xxxyxxxxxxxxxxxxxxx
     */
    tb_int_t i = 0;
    tb_int_t n = 0;
    tb_int_t t = 0;
    tb_int_t c1 = 0;
    tb_int_t c2 = 0;
    for (t = 0; t < 32; t++)
    {
        n = buffer[i >> 1];
        n = (i & 1) ? (n >> 4) : (n & 0xf);
        if (t == 16) // y
        {
            c2 = (n & 0x3) + 8;
            i++;
        }
        else if (t == 12) c2 = 4; // 4
        else // x
        {
            c2 = n;
            i++;
        }
        if (t & 1) uuid[t >> 1] = (tb_byte_t)(c1 * 16 + c2);
        c1 = c2;
    }
    return tb_true;
}

/* //////////////////////////////////////////////////////////////////////////////////////
 * implementation
 */
tb_bool_t tb_uuid_make(tb_byte_t uuid[16], tb_char_t const* name)
{
    return tb_uuid4_make(uuid, name);
}
tb_char_t const* tb_uuid_make_cstr(tb_char_t uuid_cstr[37], tb_char_t const* name)
{
    return tb_uuid4_make_cstr(uuid_cstr, name);
}
tb_bool_t tb_uuid4_make(tb_byte_t uuid[16], tb_char_t const* name)
{
    // check
    tb_assert_and_check_return_val(uuid, tb_false);

    // init seed
    union { tb_byte_t b[16]; tb_uint64_t word[2]; } s;
    if (name)
    {
        tb_uint64_t seed[2];
        tb_assert_static(sizeof(seed) == 16);
        tb_md5_make((tb_byte_t const*)name, tb_strlen(name), (tb_byte_t*)seed, 16);

        s.word[0] = tb_uuid4_xorshift128plus(seed);
        s.word[1] = tb_uuid4_xorshift128plus(seed);
    }
    else
    {
        static union { tb_byte_t b[16]; tb_uint64_t word[2]; } s_seed = {0};
        if (!s_seed.word[0] && !s_seed.word[1])
        {
            s_seed.word[0] = (tb_uint64_t)tb_uclock();
            s_seed.word[1] = (tb_uint64_t)tb_uclock();
        }

        s_seed.word[0] = tb_uuid4_xorshift128plus(s_seed.word);
        s_seed.word[1] = tb_uuid4_xorshift128plus(s_seed.word);
        s.word[0] = s_seed.word[0];
        s.word[1] = s_seed.word[1];
    }

    // generate uuid v4
    return tb_uuid4_generate(uuid, s.b);
}
tb_char_t const* tb_uuid4_make_cstr(tb_char_t uuid_cstr[37], tb_char_t const* name)
{
    // check
    tb_assert_and_check_return_val(uuid_cstr, tb_null);

    // make uuid bytes
    tb_byte_t uuid[16];
    if (!tb_uuid4_make(uuid, name)) return tb_null;

    // make uuid string
    static tb_char_t const* digits_table = "0123456789ABCDEF";
    tb_char_t* s = uuid_cstr;

    s[0] = digits_table[(uuid[0] >> 4) & 15];
    s[1] = digits_table[uuid[0] & 15];

    s[2] = digits_table[(uuid[1] >> 4) & 15];
    s[3] = digits_table[uuid[1] & 15];

    s[4] = digits_table[(uuid[2] >> 4) & 15];
    s[5] = digits_table[uuid[2] & 15];

    s[6] = digits_table[(uuid[3] >> 4) & 15];
    s[7] = digits_table[uuid[3] & 15];

    s[8] = '-';

    s[9] = digits_table[(uuid[4] >> 4) & 15];
    s[10] = digits_table[uuid[4] & 15];

    s[11] = digits_table[(uuid[5] >> 4) & 15];
    s[12] = digits_table[uuid[5] & 15];

    s[13] = '-';

    s[14] = digits_table[(uuid[6] >> 4) & 15];
    s[15] = digits_table[uuid[6] & 15];

    s[16] = digits_table[(uuid[7] >> 4) & 15];
    s[17] = digits_table[uuid[7] & 15];

    s[18] = '-';

    s[19] = digits_table[(uuid[8] >> 4) & 15];
    s[20] = digits_table[uuid[8] & 15];

    s[21] = digits_table[(uuid[9] >> 4) & 15];
    s[22] = digits_table[uuid[9] & 15];

    s[23] = '-';

    s[24] = digits_table[(uuid[10] >> 4) & 15];
    s[25] = digits_table[uuid[10] & 15];

    s[26] = digits_table[(uuid[11] >> 4) & 15];
    s[27] = digits_table[uuid[11] & 15];

    s[28] = digits_table[(uuid[12] >> 4) & 15];
    s[29] = digits_table[uuid[12] & 15];

    s[30] = digits_table[(uuid[13] >> 4) & 15];
    s[31] = digits_table[uuid[13] & 15];

    s[32] = digits_table[(uuid[14] >> 4) & 15];
    s[33] = digits_table[uuid[14] & 15];

    s[34] = digits_table[(uuid[15] >> 4) & 15];
    s[35] = digits_table[uuid[15] & 15];

    s[36] = '\0';
    return uuid_cstr;
}
