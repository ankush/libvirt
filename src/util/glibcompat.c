/*
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glibcompat.h"

/*
 * Note that because of the GLIB_VERSION_MAX_ALLOWED constant in
 * config-post.h, allowing use of functions from newer GLib via
 * this compat impl needs a little trickery to prevent warnings
 * being emitted.
 *
 * Consider a function from newer glib-X.Y that we want to use
 *
 *    int g_foo(const char *wibble)
 *
 * We must define a function with the same signature that does
 * what we need, but with a "vir_" prefix e.g.
 *
 * void vir_g_foo(const char *wibble)
 * {
 *     #if GLIB_CHECK_VERSION(X, Y, 0)
 *        g_foo(wibble)
 *     #else
 *        g_something_equivalent_in_older_glib(wibble);
 *     #endif
 * }
 *
 * The #pragma at the top of this file turns off -Wdeprecated-declarations,
 * ensuring this wrapper function impl doesn't trigger the compiler
 * warning about using too new glib APIs. Finally in glibcompat.h we can
 * add
 *
 *   #define g_foo(a) vir_g_foo(a)
 *
 * Thus all the code elsewhere in libvirt, which *does* have the
 * -Wdeprecated-declarations warning active, can call g_foo(...) as
 * normal, without generating warnings. The cost is an extra function
 * call when using new glib, but this compat code will go away over
 * time as we update the supported platforms target.
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/**
 * Adapted (to pass syntax check) from 'g_string_replace' from
 * glib-2.81.1. Drop once minimum glib is bumped to 2.68.
 *
 * g_string_replace:
 * @string: a #GString
 * @find: the string to find in @string
 * @replace: the string to insert in place of @find
 * @limit: the maximum instances of @find to replace with @replace, or `0` for
 * no limit
 *
 * Replaces the string @find with the string @replace in a #GString up to
 * @limit times. If the number of instances of @find in the #GString is
 * less than @limit, all instances are replaced. If @limit is `0`,
 * all instances of @find are replaced.
 *
 * If @find is the empty string, since versions 2.69.1 and 2.68.4 the
 * replacement will be inserted no more than once per possible position
 * (beginning of string, end of string and between characters). This did
 * not work correctly in earlier versions.
 *
 * Returns: the number of find and replace operations performed.
 *
 * Since: 2.68
 */
guint
vir_g_string_replace(GString *string,
                     const gchar *find,
                     const gchar *replace,
                     guint limit)
{
    gsize f_len, r_len, pos;
    gchar *cur, *next;
    guint n = 0;

    g_return_val_if_fail(string != NULL, 0);
    g_return_val_if_fail(find != NULL, 0);
    g_return_val_if_fail(replace != NULL, 0);

    f_len = strlen(find);
    r_len = strlen(replace);
    cur = string->str;

    while ((next = strstr(cur, find)) != NULL) {
        pos = next - string->str;
        g_string_erase(string, pos, f_len);
        g_string_insert(string, pos, replace);
        cur = string->str + pos + r_len;
        n++;
        /* Only match the empty string once at any given position, to
         * avoid infinite loops */
        if (f_len == 0) {
            if (cur[0] == '\0')
                break;
            else
                cur++;
        }
        if (n == limit)
            break;
    }

    return n;
}
