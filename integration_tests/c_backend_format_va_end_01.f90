module c_backend_format_va_end_01_m
implicit none
integer, parameter :: dp = kind(1.0d0)

contains

pure function format_real(value, format) result(str)
    real(dp), intent(in) :: value
    character(len=*), intent(in) :: format
    character(len=:), allocatable :: str
    character(len=128) :: buffer

    write(buffer, format) value
    str = trim(adjustl(buffer))
end function

end module

program c_backend_format_va_end_01
use c_backend_format_va_end_01_m, only: dp, format_real
implicit none
integer :: i
real(dp) :: value
character(len=:), allocatable :: str

do i = 1, 5000
    value = 1.0700476284695_dp + real(mod(i, 17), dp) * 1.0e-12_dp
    str = format_real(value, "(es20.13)")
    if (len_trim(str) == 0) error stop "empty formatted string"
end do

if (index(str, "E") == 0 .and. index(str, "e") == 0) then
    error stop "missing exponent"
end if

end program
