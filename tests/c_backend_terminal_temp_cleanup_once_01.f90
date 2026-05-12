module c_backend_terminal_temp_cleanup_once_01_m
implicit none

contains

subroutine shrink_suffix(x)
    integer, allocatable, intent(inout) :: x(:)

    x = x(2:)
end subroutine shrink_suffix

end module c_backend_terminal_temp_cleanup_once_01_m

program c_backend_terminal_temp_cleanup_once_01
use c_backend_terminal_temp_cleanup_once_01_m, only: shrink_suffix
implicit none

integer, allocatable :: x(:)

x = [1, 2, 3, 4]
call shrink_suffix(x)

if (size(x) /= 3) error stop
if (any(x /= [2, 3, 4])) error stop

end program c_backend_terminal_temp_cleanup_once_01
