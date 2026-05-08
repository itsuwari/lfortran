program c_backend_format_decimal_bounds_01
implicit none

integer, parameter :: dp = kind(1.0d0)
integer :: i
real(dp) :: x
character(len=32) :: buffer
character(len=:), allocatable :: s

do i = 1, 5000
    x = 1.234567_dp + real(mod(i, 19), dp) * 1.0e-9_dp
    write(buffer, "(es16.8)") x
    s = trim(adjustl(buffer))
    if (len_trim(s) == 0) error stop 1
    if (index(s, "E") == 0 .and. index(s, "e") == 0) error stop 2
end do

end program c_backend_format_decimal_bounds_01
