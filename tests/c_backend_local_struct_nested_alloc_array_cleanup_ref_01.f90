module c_backend_local_struct_nested_alloc_array_cleanup_ref_01_m
implicit none

type :: child_type
    real, allocatable :: values(:)
end type

type :: owner_type
    type(child_type), allocatable :: child
end type

contains

subroutine fill(owner)
    type(owner_type), intent(out) :: owner

    allocate(owner%child)
    allocate(owner%child%values(2))
    owner%child%values = [1.0, 2.0]
end subroutine

subroutine exercise()
    type(owner_type) :: temp_struct_var__owner

    call fill(temp_struct_var__owner)
    if (temp_struct_var__owner%child%values(2) /= 2.0) error stop
end subroutine

end module

program c_backend_local_struct_nested_alloc_array_cleanup_ref_01
use c_backend_local_struct_nested_alloc_array_cleanup_ref_01_m, only: exercise
implicit none

call exercise()
end program
