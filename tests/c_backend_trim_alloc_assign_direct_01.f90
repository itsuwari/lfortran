module c_backend_trim_alloc_assign_direct_01_m
implicit none
contains

function make_label(x) result(str)
    integer, intent(in) :: x
    character(len=:), allocatable :: str
    character(len=16) :: buffer

    write(buffer, "(i0)") x
    str = trim(buffer)
end function

end module

program c_backend_trim_alloc_assign_direct_01
use c_backend_trim_alloc_assign_direct_01_m, only: make_label
implicit none

if (make_label(42) /= "42") error stop

end program
