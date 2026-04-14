program allocatable_scalar_intent_out_01
    implicit none
    integer, allocatable :: x

    allocate(x)
    x = -1

    call fill_scalar(x)

    if (.not. allocated(x)) error stop
    if (x /= 7) error stop

contains

    subroutine fill_scalar(x)
        integer, allocatable, intent(out) :: x

        x = 7
        if (.not. allocated(x)) error stop
    end subroutine fill_scalar

end program allocatable_scalar_intent_out_01
