module c_backend_intent_out_struct_member_descriptor_cleanup_01_m
implicit none

type :: owner_type
    real, allocatable :: values(:)
end type

contains

subroutine init(owner)
    type(owner_type), intent(out) :: owner

    allocate(owner%values(2))
    owner%values = [1.0, 2.0]
end subroutine

subroutine exercise()
    type(owner_type) :: owner

    call init(owner)
    if (owner%values(2) /= 2.0) error stop
end subroutine

end module

program c_backend_intent_out_struct_member_descriptor_cleanup_01
use c_backend_intent_out_struct_member_descriptor_cleanup_01_m, only: exercise
implicit none

call exercise()
end program
