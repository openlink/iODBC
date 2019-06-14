#
# Test example for check UCS2 support
#
#
all: iodbc_ucs2_test iodbc_ucs4_test uodbc_ucs2_test


iodbc_ucs4_test: ucs_test.c
	cc -I/usr/local/iODBC/include -DODBCVER=0x0350 -DUNICODE -o iodbc_ucs4_test ucs_test.c -liodbc -ldl

iodbc_ucs2_test: ucs_test.c
	cc -I/usr/local/iODBC/include -DODBCVER=0x0350 -DUNICODE -DWIDE_UCS2 -o iodbc_ucs2_test ucs_test.c -liodbc -ldl

uodbc_ucs2_test: ucs_test.c
	cc -I/usr/include -DODBCVER=0x0350 -DUNICODE -DWIDE_UCS2 -o uodbc_ucs2_test ucs_test.c -lodbc -ldl

