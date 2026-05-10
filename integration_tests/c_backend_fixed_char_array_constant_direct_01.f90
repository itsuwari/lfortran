program c_backend_fixed_char_array_constant_direct_01
implicit none

character(len=4), parameter :: sym(4) = [character(len=4) :: "N", "H", "C", "O"]
character(len=4) :: copy(4)

copy = sym

if (sym(1) /= "N   ") error stop
if (sym(2) /= "H   ") error stop
if (sym(3) /= "C   ") error stop
if (sym(4) /= "O   ") error stop
if (copy(1) /= "N   ") error stop
if (copy(4) /= "O   ") error stop

copy(2) = "He"
if (copy(2) /= "He  ") error stop
if (sym(2) /= "H   ") error stop

call check_symbols(sym)

contains

subroutine check_symbols(values)
    character(len=4), intent(in) :: values(:)
    if (values(1) /= "N   ") error stop
    if (values(3) /= "C   ") error stop
end subroutine

end program c_backend_fixed_char_array_constant_direct_01
