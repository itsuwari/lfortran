module c_backend_module_parameter_array_no_temp_01_m
implicit none

integer, parameter :: lookup(0:6) = [1, 3, 5, 7, 9, 11, 13]

contains

subroutine pick(i, j)
integer, intent(in) :: i
integer, intent(out) :: j

j = lookup(i) + lookup(i + 1)
end subroutine

end module

program c_backend_module_parameter_array_no_temp_01
use c_backend_module_parameter_array_no_temp_01_m, only: pick
implicit none

integer :: j

call pick(2, j)
if (j /= 12) error stop

end program
