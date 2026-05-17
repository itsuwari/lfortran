module c_backend_deferred_intent_out_cleanup_01_mod
implicit none

type :: payload
    integer, allocatable :: values(:)
    integer :: tag = 7
end type

contains

subroutine initialize_value(target)
    type(payload), intent(out) :: target

    allocate(target%values(1))
    target%values(1) = 42
    target%tag = 11
end subroutine

subroutine forward_initialize(forwarded)
    type(payload), intent(out) :: forwarded

    call initialize_value(forwarded)
end subroutine

end module

program c_backend_deferred_intent_out_cleanup_01
use c_backend_deferred_intent_out_cleanup_01_mod
implicit none

type(payload) :: item

allocate(item%values(3))
item%values = [1, 2, 3]
item%tag = 5

call forward_initialize(item)

if (.not. allocated(item%values)) error stop
if (size(item%values) /= 1) error stop
if (item%values(1) /= 42) error stop
if (item%tag /= 11) error stop

end program
