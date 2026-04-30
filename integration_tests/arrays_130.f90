program arrays_130
    integer :: values(20)

    call fill_values(values)
    print *, values(1), values(2), values(20)
    if (values(1) /= 1) error stop
    if (values(2) /= 0) error stop
    if (values(20) /= 0) error stop

contains

    subroutine fill_values(out)
        integer, intent(out) :: out(:)
        integer, parameter :: max_index = 19
        integer :: scratch(0:max_index)

        scratch(:) = 7
        scratch(:) = [1, spread(0, 1, max_index)]

        out(:) = -1
        out(1) = scratch(0)
        out(2) = scratch(1)
        out(20) = scratch(19)
    end subroutine

end program
