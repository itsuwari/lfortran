module c_backend_generated_struct_member_descriptor_cleanup_ref_01_m
implicit none

type :: record_type
    real, allocatable :: values(:)
end type

type :: owner_type
    type(record_type), allocatable :: records(:)
end type

contains

subroutine use_owner(owner)
    type(owner_type), intent(in), optional :: owner

    if (present(owner)) then
        if (allocated(owner%records)) then
            if (owner%records(1)%values(1) /= 1.0) error stop
        end if
    end if
end subroutine

subroutine exercise()
    call use_owner()
end subroutine

end module

program c_backend_generated_struct_member_descriptor_cleanup_ref_01
use c_backend_generated_struct_member_descriptor_cleanup_ref_01_m, only: exercise
implicit none

call exercise()
end program
