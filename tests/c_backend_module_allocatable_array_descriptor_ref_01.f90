module c_backend_module_allocatable_array_descriptor_ref_01_m
implicit none

real, allocatable :: values(:)

contains

subroutine init_values()
    if (.not. allocated(values)) then
        allocate(values(2))
        values = [1.0, 2.0]
    end if
end subroutine

end module

program c_backend_module_allocatable_array_descriptor_ref_01
use c_backend_module_allocatable_array_descriptor_ref_01_m, only: values, init_values
implicit none

call init_values()
if (.not. allocated(values)) error stop
if (size(values) /= 2) error stop
if (abs(sum(values) - 3.0) > 1.0e-6) error stop

end program
