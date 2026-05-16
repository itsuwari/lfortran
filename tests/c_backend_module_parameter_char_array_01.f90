module c_backend_module_parameter_char_array_01_m
implicit none

character(2), parameter :: symbols(3) = ["h ", "he", "li"]

contains

subroutine symbol_index(symbol, idx)
character(2), intent(in) :: symbol
integer, intent(out) :: idx
integer :: i

idx = 0
do i = 1, size(symbols)
    if (symbol == symbols(i)) idx = i
end do
end subroutine

end module

program c_backend_module_parameter_char_array_01
use c_backend_module_parameter_char_array_01_m, only: symbol_index
implicit none

integer :: idx

call symbol_index("he", idx)
if (idx /= 2) error stop

end program
