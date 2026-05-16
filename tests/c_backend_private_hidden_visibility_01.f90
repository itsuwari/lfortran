module private_hidden_visibility_mod
implicit none
private
public :: run_hidden_visibility
contains

subroutine helper(value)
integer, intent(inout) :: value
value = value + 1
end subroutine

subroutine run_hidden_visibility(value)
integer, intent(inout) :: value
call helper(value)
end subroutine

end module

program c_backend_private_hidden_visibility_01
use private_hidden_visibility_mod, only: run_hidden_visibility
implicit none
integer :: value
value = 1
call run_hidden_visibility(value)
if (value /= 2) error stop
end program
