module c_backend_local_struct_alloc_array_descriptor_cleanup_ref_01_m
implicit none

type :: owner_type
    real, allocatable :: values(:)
end type

contains

subroutine exercise()
    type(owner_type) :: owner

    allocate(owner%values(2))
    owner%values(1) = 1.0
    owner%values(2) = 2.0
    if (owner%values(2) /= 2.0) error stop
end subroutine

end module

program c_backend_local_struct_alloc_array_descriptor_cleanup_ref_01
use c_backend_local_struct_alloc_array_descriptor_cleanup_ref_01_m, only: exercise
implicit none

call exercise()

end program
