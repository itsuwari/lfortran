module c_backend_intent_out_struct_reinit_cleanup_ref_01_m
implicit none

type payload
    integer, allocatable :: values(:)
end type

contains

subroutine init(x)
    type(payload), intent(out) :: x
    allocate(x%values(2))
    x%values = [1, 2]
end subroutine

subroutine wrap(x)
    type(payload), intent(out) :: x
    call init(x)
end subroutine

end module

program c_backend_intent_out_struct_reinit_cleanup_ref_01
use c_backend_intent_out_struct_reinit_cleanup_ref_01_m, only: payload, wrap
implicit none

type(payload) :: item

call wrap(item)
if (.not. allocated(item%values)) error stop
if (any(item%values /= [1, 2])) error stop

end program
