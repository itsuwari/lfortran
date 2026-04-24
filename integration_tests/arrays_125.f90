program arrays_125
    implicit none

    type :: holder
        real :: values(4) = 0.0
    end type

    type(holder) :: item

    call fill_values(item%values)

    if (any(item%values /= [1.0, 2.0, 3.0, 4.0])) then
        error stop "fixed-size member array was not passed as a descriptor"
    end if

contains

    subroutine fill_values(values)
        real, intent(out) :: values(:)
        integer :: i

        do i = 1, size(values)
            values(i) = real(i)
        end do
    end subroutine

end program
