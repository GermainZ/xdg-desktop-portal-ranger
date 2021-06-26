#!/usr/bin/sh
# This wrapper script is invoked by xdg-desktop-portal-ranger.
#
# Inputs:
# 1. "1" if multiple files can be chosen, "0" otherwise.
# 2. "1" if a directory should be chosen, "0" otherwise.
# 3. "1" if opening files was requested, "0" if writing to a file was
#    requested. For example, when uploading files in Firefox, this will be "1".
#    When saving a web page in Firefox, this will be "0".
# 4. If writing to a file, this is recommended path provided by the caller. For
#    example, when saving a web page in Firefox, this will be the recommended
#    path Firefox provided, such as "~/Downloads/webpage_title.html".
#    Note that if the path already exists, we keep appending "." to it until we
#    get a path that does not exist.
#
# Output:
# The script should print the selected paths to stdout, one path per line.
# If nothing is printed, then the operation is assumed to have been canceled.

multiple="$1"
directory="$2"
open="$3"
path="$4"

cmd="/usr/bin/ranger"
tmpfile="$(mktemp)"

if [ "$directory" = "1" ]; then
    cmd="$cmd --choosedir=$tmpfile --show-only-dirs"
elif [ "$multiple" = "1" ]; then
    cmd="$cmd --choosefiles=$tmpfile"
else
    cmd="$cmd --choosefile=$tmpfile"
fi

if [ "$open" = "1" ]; then
    set -- "$path"
    printf 'xdg-desktop-portal-ranger saving files tutorial

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!                 === WARNING! ===                 !!!
!!! The contents of *whatever* file you open last in !!!
!!! ranger will be *overwritten*!                    !!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

Instructions:
1) Move this file wherever you want.
2) Rename the file if needed. Remember to remove the
   trailing '.' from the filename (e.g. by pressing
   `A<Backspace><Return>`).
3) Confirm your selection by opening the file, for
   example by pressing <Enter>.

Notes:
1) This file is provided for your convenience. You
   could delete it and choose another file to overwrite
   that, for example.
2) If you quit ranger without opening a file, this file
   will be removed and the save operation aborted.
' > "$path"
fi

$cmd "$@"
if [ "$open" = "1" ] && [ ! -s "$tmpfile" ]; then
    rm "$path"
fi
cat "$tmpfile"
rm "$tmpfile"
