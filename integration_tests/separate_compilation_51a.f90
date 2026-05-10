module separate_compilation_51a
use separate_compilation_51dep, only: algorithm
implicit none
private
public :: config, get_solver

type, public :: config
    integer :: solver = algorithm%gvd
end type

contains

subroutine get_solver(self, solver)
    type(config), intent(in) :: self
    integer, intent(out) :: solver

    solver = self%solver
end subroutine

end module
