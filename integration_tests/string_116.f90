program string_116
    implicit none
    character(len=:), allocatable :: out
    character(len=8) :: str
    integer :: i

    str = "abcdef  "
    i = 3
    out = str(1:i)
    if (out /= "abc") error stop

    out(i:i) = str(2:2)
    if (out /= "abb") error stop

    if (str(i:i) /= "c") error stop
    print *, trim(out)
end program
