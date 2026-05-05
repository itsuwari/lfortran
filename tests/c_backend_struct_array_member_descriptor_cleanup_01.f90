module c_backend_struct_array_member_descriptor_cleanup_01_m
implicit none

type :: record_type
    real, allocatable :: values(:)
end type

type :: owner_type
    type(record_type), allocatable :: records(:)
end type

contains

subroutine exercise()
    type(owner_type) :: owner
    integer :: i

    allocate(owner%records(3))
    do i = 1, 3
        allocate(owner%records(i)%values(2))
        owner%records(i)%values = [real(i), real(i + 1)]
    end do

    if (owner%records(3)%values(2) /= 4.0) error stop
end subroutine

end module

program c_backend_struct_array_member_descriptor_cleanup_01
use c_backend_struct_array_member_descriptor_cleanup_01_m, only: exercise
implicit none

call exercise()
end program
