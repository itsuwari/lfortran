program separate_compilation_51
    use separate_compilation_51a, only: config, get_solver
    implicit none

    type(config) :: input
    integer :: solver

    call get_solver(input, solver)
    if (solver /= 1) error stop
end program
