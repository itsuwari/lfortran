module c_backend_alloc_char_cleanup_01_m
implicit none

contains

subroutine read_real_list(arg, val)
    character(len=:), allocatable, intent(in) :: arg
    real(8), intent(out) :: val(:)
    character(len=:), allocatable :: tmp
    integer :: i, stat

    allocate(character(len=len(arg)) :: tmp)
    do i = 1, len(arg)
        if (arg(i:i) == ",") then
            tmp(i:i) = " "
        else
            tmp(i:i) = arg(i:i)
        end if
    end do
    read(tmp, *, iostat=stat) val
    if (stat /= 0) error stop
end subroutine

end module

program c_backend_alloc_char_cleanup_01
use c_backend_alloc_char_cleanup_01_m, only: read_real_list
implicit none

character(len=:), allocatable :: arg
real(8) :: val(3)

arg = "1.5,2.5,3.5"
call read_real_list(arg, val)
if (any(abs(val - [1.5d0, 2.5d0, 3.5d0]) > 1.0d-12)) error stop

end program
