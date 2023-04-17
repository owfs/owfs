# aclocal macros specific to OWFS

dnl
dnl Check to see if the c compiler supports nested functions
dnl
AC_DEFUN([OW_CHECK_NESTED_FUNCTIONS],
[AC_REQUIRE([AC_PROG_CC])
AC_MSG_CHECKING(if compiler supports nested functions)
AC_TRY_RUN([
void f(void (*nested)())
{
    (*nested)();
}

int main(void)
{
    int a = 0;
    void nested()
    {
        a = 1;
    }
    f(nested);
    if(a != 1)
         return 1;
    return 0;
}
],
[AC_MSG_RESULT(yes)
],
AC_MSG_RESULT(no)
CFLAGS="$CFLAGS -DNO_NESTED_FUNCTIONS"
)
])
