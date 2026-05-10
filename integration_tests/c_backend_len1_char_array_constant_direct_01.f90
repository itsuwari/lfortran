program c_backend_len1_char_array_constant_direct_01
implicit none

character(len=1), parameter :: flags(4) = [character(len=1) :: "n", "N", "t", "T"]
character(len=1) :: copy(4)

copy = flags

if (flags(1) /= "n") error stop
if (flags(2) /= "N") error stop
if (flags(3) /= "t") error stop
if (flags(4) /= "T") error stop
if (.not. any(copy == "N")) error stop

copy(1) = "y"
if (copy(1) /= "y") error stop
if (flags(1) /= "n") error stop

call check_flags(flags)

contains

subroutine check_flags(values)
    character(len=1), intent(in) :: values(:)
    if (values(1) /= "n") error stop
    if (values(4) /= "T") error stop
end subroutine

end program c_backend_len1_char_array_constant_direct_01
