#
#  Autoconf macros used by OpenLink
#

AC_DEFUN([OPL_CONFIG_NICE], [
  rm -f $1
  cat >$1 <<-CONFIG_NICE_EOF
	#!$SHELL
	#
	# Created by configure for $PACKAGE_STRING
	#

CONFIG_NICE_EOF

  export_names="export"
  for var in SHELL CFLAGS CXXFLAGS CPPFLAGS LDFLAGS INCLUDES LIBS CC CXX
  do
    eval val=\$$var
    if test -n "$val"; then
      echo "$var='$val'" >> $1
      export_names="$export_names $var"
    fi
  done

  echo "$export_names" >> $1

  echo "" >> $1
  echo "# configure" >> $1

  eval "set -- $ac_configure_args"
  echo "[$]SHELL \"[$]0\" \\" >> $1
  for arg
  do
    echo "'$arg' \\" >> $1
  done
  echo '"[$]@"' >> $1
  chmod +x $1
])dnl
