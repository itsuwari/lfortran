program allocatable_character_dummy_01
    implicit none

    character(len=:), allocatable :: value

    value = "old"
    call fill(value)
    if (.not. allocated(value)) error stop "value not allocated"
    if (value /= "ok") error stop "unexpected value"

contains

    subroutine fill(arg)
        character(len=:), allocatable, intent(out) :: arg

        if (allocated(arg)) deallocate(arg)
        arg = "ok"
    end subroutine

end program
