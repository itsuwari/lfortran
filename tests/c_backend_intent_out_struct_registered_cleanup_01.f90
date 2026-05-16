module c_backend_intent_out_struct_registered_cleanup_01
implicit none

type :: payload
    integer, allocatable :: values(:)
end type

contains

subroutine reset(self)
type(payload), intent(out) :: self

allocate(self%values(2))
self%values = [1, 2]
end subroutine reset

end module c_backend_intent_out_struct_registered_cleanup_01
