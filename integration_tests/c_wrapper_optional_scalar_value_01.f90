module c_wrapper_optional_scalar_value_01
contains

    subroutine take(x, y)
        real(8), intent(in), optional :: x
        real(8), intent(out) :: y

        if (present(x)) then
            y = x
        else
            y = -1d0
        end if
    end subroutine

    subroutine test_allocated_actual()
        real(8), allocatable :: x
        real(8) :: y

        allocate(x)
        x = 3d0
        call take(x, y)
        if (abs(y - 3d0) > 1d-12) error stop 1
    end subroutine

end module

program main
    use c_wrapper_optional_scalar_value_01

    call test_allocated_actual()
end program
