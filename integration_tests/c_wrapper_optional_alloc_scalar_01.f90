module c_wrapper_optional_alloc_scalar_01
contains

    function make(flag, x) result(y)
        logical, intent(in), optional :: flag
        real(8), allocatable, intent(in), optional :: x
        real(8) :: y

        y = 1d0
        if (present(flag)) then
            if (flag) y = 2d0
        end if
        if (present(x)) then
            y = y + x
        end if
    end function

    subroutine test_absent_optional()
        real(8) :: y

        y = make(flag=.true.)
        if (abs(y - 2d0) > 1d-12) error stop 1
    end subroutine

end module

program main
    use c_wrapper_optional_alloc_scalar_01

    call test_absent_optional()
end program
