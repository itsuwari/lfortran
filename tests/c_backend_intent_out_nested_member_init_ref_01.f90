module c_backend_intent_out_nested_member_init_ref_01_m
implicit none

type inner
    integer, allocatable :: values(:)
end type

type outer
    type(inner) :: left
    type(inner) :: right
end type

contains

subroutine init(value)
    type(outer), intent(out) :: value
end subroutine

end module

program c_backend_intent_out_nested_member_init_ref_01
use c_backend_intent_out_nested_member_init_ref_01_m, only: outer, init
implicit none
type(outer) :: value
call init(value)
end program
