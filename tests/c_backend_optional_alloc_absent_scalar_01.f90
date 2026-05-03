subroutine consume(x)
    integer, optional, intent(in) :: x
    if (present(x)) error stop
end subroutine

program main
    integer, allocatable :: x
    call consume(x)
end program
