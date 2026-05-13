module c_backend_large_parameter_array_data_unit_01_mod
implicit none

integer :: i
real(8), parameter :: module_table(200) = [(real(i, 8), i = 1, 200)]

contains

subroutine sum_module_table(total)
real(8), intent(out) :: total

total = sum(module_table)
end subroutine

end module

program c_backend_large_parameter_array_data_unit_01
use c_backend_large_parameter_array_data_unit_01_mod, only: sum_module_table
implicit none

real(8) :: total, module_total

call sum_table(total)
if (abs(total - 20100.0d0) > 1.0d-12) error stop
call sum_module_table(module_total)
if (abs(module_total - 20100.0d0) > 1.0d-12) error stop

contains

subroutine sum_table(total)
real(8), intent(out) :: total
integer :: i
real(8), parameter :: table(200) = [(real(i, 8), i = 1, 200)]

total = 0.0d0
do i = 1, 200
    total = total + table(i)
end do
end subroutine

end program
