module c_wrapper_numeric_01
contains

    subroutine sink2(a)
        integer, intent(inout) :: a(2, 3)

        a(2, 3) = 42
    end subroutine

    subroutine sink3(a)
        real(8), intent(out) :: a(2, 2, 2)

        a = 0.0d0
        a(2, 2, 2) = 1.5d0
    end subroutine

    subroutine test_integer_rank_change()
        integer :: buf(6)

        buf = 0
        call sink2(buf)
        if (buf(6) /= 42) error stop 1
    end subroutine

    subroutine test_real_rank_change()
        real(8) :: buf(8)

        buf = -1.0d0
        call sink3(buf)
        if (abs(buf(8) - 1.5d0) > 1.0d-12) error stop 2
    end subroutine

end module

program main
    use c_wrapper_numeric_01

    call test_integer_rank_change()
    call test_real_rank_change()
end program
