module c_backend_fixed_char_trim_concat_output_01_m
implicit none
contains

pure function format_string_real_dp(val) result(str)
    real(8), intent(in) :: val
    character(len=:), allocatable :: str
    character(len=128) :: buffer
    integer :: stat

    write(buffer, "(es20.13)", iostat=stat) val
    if (stat == 0) then
        if (len_trim(buffer) /= 20) error stop
        str = trim(buffer)
    else
        str = "*"
    end if
end function

subroutine check_message(msg)
    character(len=*), intent(in) :: msg

    if (len(msg) /= 48) error stop
    if (msg /= "total energy             -3.4980794816227E+01 Eh") error stop
end subroutine

end module

program c_backend_fixed_char_trim_concat_output_01
use c_backend_fixed_char_trim_concat_output_01_m, only: format_string_real_dp, check_message
implicit none

call check_message("total energy" // repeat(" ", 13) // &
    format_string_real_dp(-34.980794816227d0) // " Eh")

if (len(format_string_real_dp(-34.980794816227d0)) /= 20) error stop

end program
