program optional_alloc_struct_arg_01
    implicit none

    type :: payload
        integer :: value = -1
    end type

    type :: holder
        type(payload), allocatable :: item
    end type

    type(holder) :: h

    call check_payload(h%item, .false., -1)

    allocate(h%item)
    h%item%value = 42
    call check_payload(h%item, .true., 42)

contains

    subroutine check_payload(item, expected_present, expected_value)
        type(payload), intent(in), optional :: item
        logical, intent(in) :: expected_present
        integer, intent(in) :: expected_value

        if (present(item) .neqv. expected_present) error stop 1
        if (present(item)) then
            if (item%value /= expected_value) error stop 2
        end if
    end subroutine

end program
