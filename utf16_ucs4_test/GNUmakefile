#
# Test example for check UCS2 support
#
#
all: ucs2_test ucs4_test


ucs4_test: ucs_test.c
	cc -I/usr/local/iODBC/include -DODBCVER=0x0350 -DUNICODE -o ucs4_test ucs_test.c -liodbc -ldl

ucs2_test: ucs_test.c
	cc -I/usr/local/iODBC/include -DODBCVER=0x0350 -DUNICODE -DWIDE_UCS2 -o ucs2_test ucs_test.c -liodbc -ldl

