module array_constructor_07_mod
implicit none

character(len=*), parameter :: local_header_start(*) = [ &
    char(80), char(75), char(3), char(4), char(20), char(0)]
character(len=*), parameter :: global_header_start(*) = [ &
    char(80), char(75), char(1), char(2), char(20), char(0), &
    local_header_start(5:)]

contains

subroutine check_header()
    if (any(global_header_start(:4) /= [char(80), char(75), char(1), char(2)])) then
        error stop "array_constructor_07 failed"
    end if
end subroutine

end module

program array_constructor_07
use array_constructor_07_mod
implicit none

call check_header()

end program
