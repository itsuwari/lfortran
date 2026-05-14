module c_backend_large_logical_parameter_array_data_unit_01_mod
implicit none

integer :: i
logical, parameter :: module_mask(256) = [(mod(i, 3) == 0, i = 1, 256)]

contains

subroutine count_module_mask(total)
integer, intent(out) :: total
integer :: j

total = 0
do j = 1, 256
    if (module_mask(j)) total = total + 1
end do
end subroutine

end module

program c_backend_large_logical_parameter_array_data_unit_01
use c_backend_large_logical_parameter_array_data_unit_01_mod, only: count_module_mask
implicit none

integer :: total, module_total

call count_table(total)
if (total /= 51) error stop
call count_module_mask(module_total)
if (module_total /= 85) error stop

contains

subroutine count_table(total)
integer, intent(out) :: total
integer :: j
logical, parameter :: table(256) = [(mod(j, 5) == 0, j = 1, 256)]

total = 0
do j = 1, 256
    if (table(j)) total = total + 1
end do
end subroutine

end program
